#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <filesystem>

const unsigned char XOR_KEY = 0xFF;
const char* STASH_FLAG = "STASH";

using namespace std;

void stash(const char* filename);
void restore(const char* filename);
bool checkFlag(fstream &file);
void doxor(unsigned char* buffer); // побитовое изменение байтов
void undoxor(unsigned char* buffer);
void setFlag(fstream &file);


int main(int argc, char *argv[]) {

    struct stat info{};

    if (argc != 3) {
        printf("Usage: %s <stash|restore> <path to file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *mode = argv[1];
    const char *filename = argv[2];

    if (stat(argv[2], &info) != 0) {
        printf("Unable to get file info. File does not exist\n");
        exit(EXIT_FAILURE);
    }

    if (!S_ISREG(info.st_mode)) {
        printf("Unable to stash. Not a regular file\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(mode, "stash") == 0) {
        stash(filename);
    } else if (strcmp(mode, "restore") == 0) {
        restore(filename);
    } else {
        printf("Unknown mode: %s\n", mode);
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

// изменить первые 4 магических байта. Этого достатосно и для случая, когда магических байтов больше.
void stash(const char *filename) {
    // Открытие файла для чтения и записи
    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        printf("Ошибка открытия файла: %s",filename);
        exit(EXIT_FAILURE);
    }

    if (checkFlag(file)) {
        printf("Already stashed: %s\n",filename);
        exit(EXIT_FAILURE);
    }

    // Чтение первых 4 байтов
    unsigned char buffer[4];
    file.read(reinterpret_cast<char*>(buffer), 4);

    // Выполнение XOR
    doxor(buffer);

    file.seekp(0);                                  // Перемещение указателя в начало файла
    file.write(reinterpret_cast<char*>(buffer), 4);  // Запись измененных 4 байтов обратно в файл
    file.seekp(0, ios::end);                   // Перемещение указателя в конец файла
    file.write(STASH_FLAG, strlen(STASH_FLAG));         // Запись флага "Stash" в конец файла
    file.close();
}

void doxor(unsigned char* buffer) {
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] ^= XOR_KEY;
    }
}

void undoxor(unsigned char* buffer) {
    doxor(buffer);
}

bool checkFlag(fstream &file) {
    size_t flagLength = strlen(STASH_FLAG);

    // Перемещение указателя на позицию перед флагом
    file.seekg(-flagLength, ios::end);

    // Считывание последних flagLength байтов
    char buffer[flagLength];
    file.read(buffer, flagLength);
    file.seekg(0, ios::beg);

    // Сравнение считанных байтов с флагом
    if (memcmp(buffer, STASH_FLAG, flagLength) == 0) {
        return true; // Флаг найден
    }

    return false; // Флаг не найден
}

void restore(const char* filename) {
    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        printf("Ошибка открытия файла: %s",filename);
        exit(EXIT_FAILURE);
    }

    if (!checkFlag(file)) {
        printf("Is not stashed: %s\n",filename);
        exit(EXIT_FAILURE);
    }

    // Чтение первых 4 байтов
    unsigned char buffer[4];
    file.read(reinterpret_cast<char*>(buffer), 4);
    undoxor(buffer);

    file.seekp(0);                                  // Перемещение указателя в начало файла
    file.write(reinterpret_cast<char*>(buffer), 4);  // Запись измененных 4 байтов обратно в файл

    // Перемещение указателя на позицию перед флагом
    file.seekg(-strlen(STASH_FLAG), ios::end);
    long originFileSize = file.tellg();
    file.seekg(0);

    unsigned char originBuffer[originFileSize]; // буфер под оригинальный массив байтов
    file.read(reinterpret_cast<char*>(originBuffer), originFileSize);
    file.close();

    // Открываем файл повторно для перезаписи
    file.open(filename, ios::binary | ios::out | ios::trunc);
    if (!file.is_open()) {
        printf("Ошибка открытия файла: %s",filename);
        exit(EXIT_FAILURE);
    }

    // пишем все байты за исключением относящихся к флагу STASH_FLAG
    file.write(reinterpret_cast<char*>(originBuffer), originFileSize);
    file.close();
}