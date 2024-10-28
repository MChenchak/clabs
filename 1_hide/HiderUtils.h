#include <filesystem>

namespace HiderUtils {

    int moveFileToDarkDirectory(std::filesystem::path source, std::string darkPath);
    bool validatePath(const std::string &userPath, std::error_code &error_code);
    void createDarkDirectory(std::filesystem::path path);

};


