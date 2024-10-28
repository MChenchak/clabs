#include <iostream>
#include "HiderUtils.h"

namespace hu = HiderUtils;

const std::string DARK_PATH = "./.dark-dirr";

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Use: " << argv[0] << " <path to file>" << std::endl;
        return -1;
    }
    const std::string userPath = argv[1];
    std::error_code error_code;
    if (!hu::validatePath(userPath, error_code)) {
        std::cerr << "Validation failure with error: " << error_code.message() << std::endl;
        return -1;
    }

    hu::createDarkDirectory(DARK_PATH);

    if (hu::moveFileToDarkDirectory(userPath, DARK_PATH) != 0) {
        return -1;
    }

    return 0;
}