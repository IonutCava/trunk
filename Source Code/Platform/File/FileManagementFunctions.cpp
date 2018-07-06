#include "stdafx.h"

#include "Core/Headers/Application.h"
#include "Core/Headers/StringHelper.h"

namespace Divide {

bool writeFile(const stringImpl& filePath, const bufferPtr content, size_t length, FileType fileType) {
    if (!filePath.empty() && content != nullptr && length > 0) {
        std::ofstream outputFile(filePath.c_str(), fileType == FileType::BINARY
                                                             ? std::ios::out | std::ios::binary
                                                             : std::ios::out);
        outputFile.write(reinterpret_cast<Byte*>(content), length);
        outputFile.close();
        return outputFile.good();
    }

    return false;
}

FileWithPath splitPathToNameAndLocation(const stringImpl& input) {
    size_t pathNameSplitPoint = input.find_last_of('/') + 1;
    if (pathNameSplitPoint == 0) {
        pathNameSplitPoint = input.find_last_of("\\") + 1;
    }
    return FileWithPath
    {
        input.substr(pathNameSplitPoint, stringImpl::npos),
        input.substr(0, pathNameSplitPoint)
    };
}

//ref: https://stackoverflow.com/questions/18100097/portable-way-to-check-if-directory-exists-windows-linux-c
bool pathExists(const char* filePath) {
    struct stat info;

    if (stat(filePath, &info) != 0) {
        return false;
    } else if (info.st_mode & S_IFDIR) {
        return true;
    }

    return false;
}

bool fileExists(const char* filePathAndName) {
    return std::ifstream(filePathAndName).good();
}

bool createFile(const char* filePathAndName, bool overwriteExisting) {
    if (overwriteExisting && fileExists(filePathAndName)) {
        return std::ifstream(filePathAndName, std::fstream::in | std::fstream::trunc).good();
    }

    const SysInfo& systemInfo = const_sysInfo();
    createDirectories((systemInfo._pathAndFilename._path + "/" + splitPathToNameAndLocation(filePathAndName)._path).c_str());

    return std::ifstream(filePathAndName, std::fstream::in).good();

}

bool hasExtension(const stringImpl& filePath, const stringImpl& extension) {
    stringImpl ext("." + extension);
    return Util::CompareIgnoreCase(Util::GetTrailingCharacters(filePath, ext.length()), ext);
}

}; //namespace Divide
