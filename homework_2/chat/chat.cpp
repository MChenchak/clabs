#include <iostream>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define BROADCAST_IP "255.255.255.255"

void receiveMessages(int socket_fd) {
    char buffer[BUFFER_SIZE];
    sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int received = recvfrom(socket_fd, buffer, BUFFER_SIZE - 1, 0, (sockaddr*)&sender_addr, &sender_addr_len);
        if (received > 0) {
            buffer[received] = '\0';
            std::cout << "Получено: " << buffer << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Пример использования: " << argv[0] << " <номер_порта>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    // Создание сокета
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Ошибка создания сокета");
        return 1;
    }

    // Разрешение широковещательной отправки
    int broadcast_enable = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Не удалось установить параметр широковещательно канала");
        close(socket_fd);
        return 1;
    }

    // Привязка сокета к указанному порту
    sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port);

    if (bind(socket_fd, (sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("Ошибка привязки");
        close(socket_fd);
        return 1;
    }

    // Запуск потока для приема сообщений
    std::thread receiver(receiveMessages, socket_fd);
    receiver.detach();

    // Подготовка адреса для отправки сообщений
    sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);
    broadcast_addr.sin_port = htons(port);

    // Отправка сообщений
    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (sendto(socket_fd, message.c_str(), message.size(), 0, (sockaddr*)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
            perror("Ошибка отправки сообщения");
        }
    }

    close(socket_fd);
    return 0;
}