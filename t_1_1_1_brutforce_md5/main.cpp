#include <iostream>
#include <openssl/evp.h>
#include <openssl/provider.h>
#include <string>
#include <vector>
#include <omp.h>

// Функция для вычисления MD5 хэша строки с использованием OpenSSL 3.0
std::string calculate_md5(const std::string &input) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_create();  // Создание контекста для хэширования
    if (mdctx == nullptr) {
        std::cerr << "Ошибка создания контекста MD5" << std::endl;
        exit(1);
    }

    EVP_MD *md = EVP_MD_fetch(nullptr, "MD5", nullptr);  // Получаем MD5 из провайдера
    if (md == nullptr) {
        std::cerr << "Ошибка инициализации MD5" << std::endl;
        EVP_MD_CTX_destroy(mdctx);
        exit(1);
    }

    if (EVP_DigestInit_ex(mdctx, md, nullptr) != 1) {
        std::cerr << "Ошибка инициализации контекста MD5" << std::endl;
        EVP_MD_free(md);
        EVP_MD_CTX_destroy(mdctx);
        exit(1);
    }

    if (EVP_DigestUpdate(mdctx, input.c_str(), input.size()) != 1) {
        std::cerr << "Ошибка обновления MD5" << std::endl;
        EVP_MD_free(md);
        EVP_MD_CTX_destroy(mdctx);
        exit(1);
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    if (EVP_DigestFinal_ex(mdctx, digest, &digest_len) != 1) {
        std::cerr << "Ошибка финализации MD5" << std::endl;
        EVP_MD_free(md);
        EVP_MD_CTX_destroy(mdctx);
        exit(1);
    }

    EVP_MD_free(md);
    EVP_MD_CTX_destroy(mdctx);

    // Конвертация хэша в строку hex
    char md5string[33];
    for (int i = 0; i < digest_len; ++i) {
        sprintf(&md5string[i * 2], "%02x", digest[i]);
    }

    return {md5string};
}

// Рекурсивная функция для перебора всех возможных строк заданной длины
void brute_force(const std::string &md5_hash, const std::string &alphabet, std::string &current, int max_length, bool &found) { // NOLINT(*-no-recursion)
    if (found) return;
    // printf("%s\n", current.c_str());

    if (current.size() == max_length) {
        std::string hash = calculate_md5(current);
        if (hash == md5_hash) {
            printf("Найден исходный текст: \"%s\"\n", current.c_str());
            found = true;
        }
        return;
    }

    for (char c : alphabet) {
        if (found) break;
        current.push_back(c);
        brute_force(md5_hash, alphabet, current, max_length, found);
        current.pop_back();
    }
}

// Параллельный взлом MD5 хэша
void parallel_md5_crack(const std::string &md5_hash, const std::string &alphabet, int max_length) {
    bool found = false;
    std::vector<std::string> prefixes;

    // Создаем начальные префиксы для параллельного выполнения
    for (char c : alphabet) {
        prefixes.push_back(std::string(1, c));
    }
    printf("Max threads: %d\n", omp_get_max_threads());
    #pragma omp parallel for shared(found) num_threads(omp_get_max_threads())
    for (auto current : prefixes) {
        if (!found) {
            brute_force(md5_hash, alphabet, current, max_length, found);
        }
    }

    if (!found) {
        std::cout << "Не удалось найти совпадение." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <md5_hash>" << std::endl;
        return 1;
    }

    std::string md5_hash = argv[1];
    std::string alphabet = "abcdefghijklmnopqrstuvwxyz0123456789";  // Символы a-z и цифры 0-9
    int max_length = 5;  // Максимальная длина пароля

    parallel_md5_crack(md5_hash, alphabet, max_length);

    return 0;
}
