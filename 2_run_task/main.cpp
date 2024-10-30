#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [args...]\n", argv[0]);
        return 1;
    }

    printf("Запускаем дочерний процесс: ");
    for (int i = 1; i < argc; ++i) {
        printf("%s ", argv[i]);
    }

    // Создаем дочерний процесс
    pid_t pid = fork();

    if (pid < 0) {
        perror("Failed to fork");
        return 1;
    } else if (pid == 0) {
        // Это дочерний процесс

        int out_fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd == -1) {
            perror("Failed to open out.txt");
            return 1;
        }

        int err_fd = open("err.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (err_fd == -1) {
            perror("Failed to open err.txt");
            close(out_fd);
            return 1;
        }

        // Перенаправляем stdout и stderr
        if (dup2(out_fd, STDOUT_FILENO) == -1) {
            perror("Failed to redirect stdout");
            close(out_fd);
            close(err_fd);
            return 1;
        }

        if (dup2(err_fd, STDERR_FILENO) == -1) {
            perror("Failed to redirect stderr");
            close(out_fd);
            close(err_fd);
            return 1;
        }

        // Закрываем файловые дескрипторы, так как они уже не нужны
        close(out_fd);
        close(err_fd);

        // Запускаем программу с аргументами
        execvp(argv[1], &argv[1]);

        // Если execvp вернулся, значит произошла ошибка
        perror("Failed to execute program");
        return 1;
    } else {
        // Это родительский процесс
        int status;
        waitpid(pid, &status, 0); // Ожидаем завершения дочернего процесса

        if (WIFEXITED(status)) {
            printf("Child process exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child process terminated by signal %d\n", WTERMSIG(status));
        } else {
            printf("Child process did not exit normally\n");
        }
    }

    return 0;
}
