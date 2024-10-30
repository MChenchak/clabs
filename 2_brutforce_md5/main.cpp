#include <iostream>
#include <string>

#include "pthread_bruteforce.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <md5_hash>" << std::endl;
        return 1;
    }

    std::string md5_hash = argv[1];
    std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int max_length = 4;  // длина пароля

    std::string result;

    result = pthread_md5_crack(md5_hash, alphabet, max_length);

    if (result.empty()) {
        std::cout << "Не удалось найти совпадение." << std::endl;
    } else {
            printf("Найден исходный текст: \"%s\"\n", result.c_str());
    }

    return 0;
}
