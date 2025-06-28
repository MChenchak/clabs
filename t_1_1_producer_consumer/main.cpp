#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

constexpr auto SHARED_MEMORY_NAME = "/shm_fe4bfO3y";
constexpr auto SEM_EMPTY_NAME = "/sem_empty_fe4bfO3y";
constexpr auto SEM_FULL_NAME = "/sem_full_fe4bfO3y";
constexpr auto SHARED_MEMORY_SIZE = 1024 * 1024; // 1 MB
constexpr auto BUFFER_SIZE = 1024;

void write_to_shm(const char* file_name,
                  size_t file_name_size,
                  const char* file_content,
                  const long file_size,
                  char *shared_memory) {
    file_name_size++; // для терминального нуля
    char *shm_adr = shared_memory;
    memcpy(shm_adr, &file_size, sizeof(file_size)); shm_adr += sizeof(file_size);
    memcpy(shm_adr, &file_name_size, sizeof(file_name_size)); shm_adr += sizeof(file_name_size);
    memcpy(shm_adr, file_name, file_name_size);
    shm_adr[file_name_size - 1] = 0;
    shm_adr += file_name_size;

    memcpy(shm_adr, file_content, file_size);

}

int producer(const char *directory_path) {
    // Открытие семафоров
    sem_t *sem_empty = sem_open(SEM_EMPTY_NAME, O_CREAT, 0666, 1);
    sem_t *sem_full = sem_open(SEM_FULL_NAME, O_CREAT, 0666, 0);

    // Открытие или создание разделяемой памяти
    const int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SHARED_MEMORY_SIZE);
    char *shared_memory = static_cast<char *>(
        mmap(nullptr, SHARED_MEMORY_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0));

    DIR *dir = opendir(directory_path);
    if (dir == nullptr) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            char file_path[BUFFER_SIZE];
            snprintf(file_path, sizeof(file_path), "%s/%s", directory_path, entry->d_name);
            FILE *file = fopen(file_path, "rb");
            if (!file) {
                perror("fopen");
                continue;
            }

            // Определение размера данных для чтения
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);

            size_t file_name_size = strlen(entry->d_name) + 1;
            size_t shared_free_size = SHARED_MEMORY_SIZE - sizeof(file_size) - sizeof(file_name_size) - file_name_size;
            if (file_size > shared_free_size) {
                file_size = static_cast<long>(shared_free_size);
            }

            // Чтение файла
            char *file_content = static_cast<char *>(malloc(file_size));
            fread(file_content, 1, file_size, file);

            // Запись данных в разделяемую память
            sem_wait(sem_empty);
            write_to_shm(entry->d_name, strlen(entry->d_name),
                file_content, file_size, shared_memory);
            sem_post(sem_full);
            printf("Sent %7ld bytes of file \"%s\"\n", file_size, entry->d_name);

            free(file_content);
            fclose(file);
        }
    }
    printf("All done.");
    sem_wait(sem_empty);
    write_to_shm("", 0, "", 0, shared_memory);
    sem_post(sem_full);

    closedir(dir);
    munmap(shared_memory, SHARED_MEMORY_SIZE);
    close(shm_fd);

    sem_close(sem_empty);
    sem_close(sem_full);

    return 0;
}

int consumer(const char *directory_path) {

    // Открытие семафоров
    sem_t *sem_empty = sem_open(SEM_EMPTY_NAME, 0);
    sem_t *sem_full = sem_open(SEM_FULL_NAME, 0);

    // Открытие разделяемой памяти
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);
    char *shared_memory = static_cast<char *>(
        mmap(nullptr, SHARED_MEMORY_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0));

    int file_index = 0;

    // Уведомление производителю, что память пуста, для его запуска
    sem_post(sem_empty);

    for (;;) {
        char file_output_path[BUFFER_SIZE];
        // Ожидание данных от производителя
        sem_wait(sem_full);

        char* shm_adr = shared_memory;
        auto file_size = reinterpret_cast<long *>(shm_adr); shm_adr += sizeof(long);
        auto file_name_size = reinterpret_cast<size_t *>(shm_adr); shm_adr += sizeof(size_t);
        auto file_name = shm_adr; shm_adr += *file_name_size;
        auto file_content = shm_adr;
        if (*file_name_size == 1 && *file_size == 0) {
            break;
        }
            printf("Got %7ld bytes of file \"%s\"\n", *file_size, file_name);

        // Чтение данных из разделяемой памяти
        snprintf(file_output_path, sizeof(file_output_path), "%s/%s", directory_path, file_name);
        FILE *file = fopen(file_output_path, "wb");
        if (!file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        // читаем максимум 1MB данных, что требует улучшений для обработки реальных файлов
        fwrite(file_content, 1, *file_size, file);

        fclose(file);
        file_index++;

        // Уведомление производителю, что память пуста
        sem_post(sem_empty);
    }

    munmap(shared_memory, SHARED_MEMORY_SIZE);
    close(shm_fd);

    sem_close(sem_empty);
    sem_close(sem_full);

    return 0;
}

void usage(const char *app_name) {
    printf("Usage: %s produce|consume <directory_path>\n", app_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage(argv[0]);
    }

    if (strcmp(argv[1], "produce") == 0) {
        return producer(argv[2]);
    }
    if (strcmp(argv[1], "consume") == 0) {
        return consumer(argv[2]);
    }
    usage(argv[0]);
    return EXIT_FAILURE;
}