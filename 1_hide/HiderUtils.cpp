#include "HiderUtils.h"

namespace fs = std::filesystem;

int HiderUtils::moveFileToDarkDirectory(fs::path source, std::string darkPath) {
    const std::string fileName = source.filename().string();
    std::string resultPath = darkPath + "/" + fileName;

    if (std::rename(source.c_str(), resultPath.c_str())) {
        std::perror("File moving end with error");
        return -1;
    }

    return 0;
};

bool HiderUtils::validatePath(const std::string &userPath, std::error_code &error_code) {
    if (std::filesystem::is_directory(userPath, error_code)) {
        error_code = std::make_error_code(std::errc::is_a_directory);
        return false;
    }

    if (std::filesystem::is_regular_file(userPath, error_code)) {
        return true;
    }

    return false;
};

void HiderUtils::createDarkDirectory(std::filesystem::path path) {
    fs::create_directory(path);
    fs::permissions(
            path,
            std::filesystem::perms::owner_exec |
            std::filesystem::perms::owner_write |
            std::filesystem::perms::group_exec |
            std::filesystem::perms::group_write,
            std::filesystem::perm_options::add
    );
    fs::permissions(
            path,
            std::filesystem::perms::owner_read |
            std::filesystem::perms::group_read,
            std::filesystem::perm_options::remove
    );
};
