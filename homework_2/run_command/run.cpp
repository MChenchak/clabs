#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <signal.h>
#include <filesystem>
#include <poll.h>
#include <sys/wait.h>
#include <atomic>

constexpr int MAX_CLIENTS = 1;
constexpr int BUFFER_SIZE = 4096;
constexpr int TIMEOUT_SECONDS = 10;

// Глобальный счетчик активных клиентов
std::atomic<int> active_clients(0);

void execute_command(int client_socket, const std::string &command, const std::vector<std::string> &args) {
    std::ostringstream response;

    // Проверяем доступность программы в PATH
    std::string program_path;
    bool found = false;
    const char* path_env = std::getenv("PATH");
    if (path_env) {
        std::istringstream path_stream(path_env);
        std::string path;
        while (std::getline(path_stream, path, ':')) {
            std::filesystem::path full_path = std::filesystem::path(path) / command;
            if (std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path)) {
                program_path = full_path.string();
                found = true;
                break;
            }
        }
    }

    if (!found) {
        response << "Error: Program not found in PATH\n";
        send(client_socket, response.str().c_str(), response.str().size(), 0);
        return;
    }

    // Создаем пайпы для вывода stdout и stderr
    int stdout_pipe[2], stderr_pipe[2];
    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) { // Ребенок
        // Перенаправляем stdout и stderr в пайпы
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);

        // Формируем массив аргументов
        std::vector<const char*> exec_args;
        exec_args.push_back(program_path.c_str());
        for (const auto& arg : args) {
            exec_args.push_back(arg.c_str());
        }
        exec_args.push_back(nullptr);

        execvp(exec_args[0], const_cast<char* const*>(exec_args.data()));

        // Если execvp не выполнился, выходим с ошибкой
        perror("execvp");
        exit(EXIT_FAILURE);
    } else { // Родитель
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        // Устанавливаем таймаут
        struct pollfd fds[2];
        fds[0].fd = stdout_pipe[0];
        fds[0].events = POLLIN;
        fds[1].fd = stderr_pipe[0];
        fds[1].events = POLLIN;

        std::string stdout_output, stderr_output;
        char buffer[BUFFER_SIZE];
        bool timed_out = false;
        int status;

        auto start_time = std::chrono::steady_clock::now();

        while (true) {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > TIMEOUT_SECONDS) {
                timed_out = true;
                kill(pid, SIGKILL);
                break;
            }

            int ret = poll(fds, 2, 1000);
            if (ret > 0) {
                if (fds[0].revents & POLLIN) {
                    ssize_t count = read(stdout_pipe[0], buffer, sizeof(buffer));
                    if (count > 0) {
                        stdout_output.append(buffer, count);
                    }
                }
                if (fds[1].revents & POLLIN) {
                    ssize_t count = read(stderr_pipe[0], buffer, sizeof(buffer));
                    if (count > 0) {
                        stderr_output.append(buffer, count);
                    }
                }
            } else if (ret == -1) {
                perror("poll");
                break;
            }

            if (waitpid(pid, &status, WNOHANG) > 0) {
                break;
            }
        }

        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        if (timed_out) {
            response << "Error: Command timed out\n";
        } else if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            response << "Exit code: " << exit_code << "\n";
            response << "Stdout:\n" << stdout_output;
            if (!stderr_output.empty()) {
                response << "\nStderr:\n" << stderr_output;
            }
        } else {
            response << "Error: Command terminated abnormally\n";
        }

        send(client_socket, response.str().c_str(), response.str().size(), 0);
    }
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';

        std::istringstream request(buffer);
        std::string command;
        request >> command;

        std::vector<std::string> args;
        std::string arg;
        while (request >> arg) {
            args.push_back(arg);
        }

        execute_command(client_socket, command, args);
    }
    close(client_socket);
    --active_clients;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return EXIT_FAILURE;
    }

    // Если нужно использовать порт меньше 1024, то надо запускать через привилегированного пользователя
    int port = std::stoi(argv[1]);
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    sockaddr_in server_addr{};
    // Выбираем ipv4
    server_addr.sin_family = AF_INET;
    // Слушаем на любом сетевом интерфейсе (0.0.0.0)
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return EXIT_FAILURE;
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("listen");
        return EXIT_FAILURE;
    }

    std::cout << "Server is listening on port " << port << "...\n";

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        ++active_clients;
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept");
            continue;
        }
        if (active_clients > MAX_CLIENTS) {
            std::cout << "Maximum client limit reached. Rejecting new connection.\n";
            close(client_socket);
            --active_clients;
        } else {
            std::thread(handle_client, client_socket).detach();
        }
    }

    close(server_socket);
    return EXIT_SUCCESS;
}
