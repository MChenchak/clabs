#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("run detached process: ");
    for (int i = 1; i < argc; ++i) {
        printf("%s ", argv[i]);
    }
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Дочерний процесс
    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    // Перенаправление stdin, stdout и stderr в /dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > STDERR_FILENO) {
        close(fd);
    }

    // Запуск программы
    execvp(argv[1], &argv[1]);

    perror("detached process run error");
    exit(EXIT_FAILURE);
}
