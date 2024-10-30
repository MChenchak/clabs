#include <openssl/evp.h>
#include <openssl/provider.h>
#include <iostream>

#include "calc_md5.h"


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
