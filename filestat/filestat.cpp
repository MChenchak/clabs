#include <filesystem>
#include <iostream>
#include <unordered_map>

namespace fs = std::filesystem;

// Получить статистику для указанной директории
std::unordered_map<fs::file_type, int> calcFstat(const fs::path *);

// Преобразовать значения типов директорий в человекочитаемый вид
std::string get_type_string(const fs::file_type *);

// Вывод в стандартный поток вывода информации о количестве файлов каждого типа в текущей директории
void printFstat(const std::unordered_map<fs::file_type, int> *);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Please provide a target directory as a 1st argument" << std::endl;
        return EXIT_FAILURE;
    }

    if (!fs::is_directory(argv[1]))
    {
        std::cout << "Given path is not a directory" << std::endl;
        return EXIT_FAILURE;
    }

    const fs::path path = fs::path(argv[1]);
    const std::unordered_map<fs::file_type, int> filetype_counter = calcFstat(&path);
    printFstat(&filetype_counter);

    return EXIT_SUCCESS;
}

std::unordered_map<fs::file_type, int> calcFstat(const fs::path *path)
{
    std::unordered_map<fs::file_type, int> filetype_counter;

    for (auto const &dir_entry : std::filesystem::directory_iterator(*path)) {

        if (dir_entry.is_symlink()) {
            ++filetype_counter[fs::file_type::symlink]; // для символических ссылок функция std::filesystem::file_status::type возвращает тип файла,
                                                        // на который ссылается символическая ссылка, а не сам тип ссылки.

            continue;
        }

        ++filetype_counter[dir_entry.status().type()];
    }

    return filetype_counter;
}

std::string get_type_string(const fs::file_type ftype)
{
    switch (ftype)
    {
    case fs::file_type::none        : return std::string("`not-evaluated-yet` type: ");
    case fs::file_type::not_found   : return std::string("does not exist");
    case fs::file_type::regular     : return std::string("regular file: ");
    case fs::file_type::directory   : return std::string("directory: ");
    case fs::file_type::symlink     : return std::string("symlink: ");
    case fs::file_type::block       : return std::string("block device: ");
    case fs::file_type::character   : return std::string("character device: ");
    case fs::file_type::fifo        : return std::string("named IPC pipe: ");
    case fs::file_type::socket      : return std::string("named IPC socket: ");
    case fs::file_type::unknown     : return std::string("`unknown` type: ");
    }
}

void printFstat(const std::unordered_map<fs::file_type, int> *filetype_counter)
{
    for (auto const &kv : *filetype_counter)
        std::cout << get_type_string(kv.first) << " " << kv.second << std::endl;
}