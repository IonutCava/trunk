#include "Headers/FileManagement.h"

#include "Core/Headers/StringHelper.h"
#include <fstream>

namespace Divide {

bool readFile(const stringImpl& filePath, stringImpl& contentOut, FileType fileType) {
    if (!filePath.empty()) {
        std::ifstream inFile(filePath.c_str(), fileType == FileType::BINARY
                                                            ? std::ios::in | std::ios::binary
                                                            : std::ios::in);

        if (!inFile.eof() && !inFile.fail()) {
            inFile.seekg(0, std::ios::end);
            size_t fsize = inFile.tellg();
            inFile.seekg(0, std::ios::beg);
            contentOut.reserve(fsize);
            contentOut.assign((std::istreambuf_iterator<char>(inFile)),
                               std::istreambuf_iterator<char>());
            return true;
        }

        inFile.close();
    };

    return false;
}

bool readFile(const stringImpl& filePath, vectorImpl<Byte>& contentOut, FileType fileType) {
    stringImpl content;
    contentOut.resize(0);
    if (readFile(filePath, content, fileType)) {
        contentOut.insert(std::cbegin(contentOut), std::cbegin(content), std::cend(content));
        return true;
    }

    return false;
}

bool readFile(const stringImpl& filePath, vectorImpl<U8>& contentOut, FileType fileType) {
    stringImpl content;
    contentOut.resize(0);
    if (readFile(filePath, content, fileType)) {
        contentOut.insert(std::cbegin(contentOut), std::cbegin(content), std::cend(content));
        return true;
    }
    return false;
}

bool readFile(const stringImpl& filePath, vectorImplFast<U8>& contentOut, FileType fileType) {
    stringImpl content;
    contentOut.resize(0);
    if (readFile(filePath, content, fileType)) {
        contentOut.insert(std::cbegin(contentOut), std::cbegin(content), std::cend(content));
        return true;
    }
    return false;
}

bool readFile(const stringImpl& filePath, vectorImplFast<Byte>& contentOut, FileType fileType) {
    stringImpl content;
    contentOut.resize(0);
    if (readFile(filePath, content, fileType)) {
        contentOut.insert(std::cbegin(contentOut), std::cbegin(content), std::cend(content));
        return true;
    }
    return false;
}

bool writeFile(const stringImpl& filePath, const char* content, size_t length, FileType fileType) {
    if (!filePath.empty() && content != nullptr && length > 0) {
        std::ofstream outputFile(filePath.c_str(), fileType == FileType::BINARY
                                                                ? std::ios::out | std::ios::binary
                                                                : std::ios::out);
        outputFile.write(content, length);
        outputFile.close();
        return outputFile.good();
    }

    return false;
}

bool writeFile(const stringImpl& filePath, const char* content, FileType fileType) {
    return writeFile(filePath, content, strlen(content), fileType);
}

bool writeFile(const stringImpl& filePath, const vectorImpl<U8>& content, FileType fileType) {
    return writeFile(filePath, content, content.size(), fileType);
}

bool writeFile(const stringImpl& filePath, const vectorImpl<U8>& content, size_t length, FileType fileType) {
    return writeFile(filePath, reinterpret_cast<const char*>(content.data()), length, fileType);
}

bool writeFile(const stringImpl& filePath, const vectorImpl<Byte>& content, FileType fileType) {
    return writeFile(filePath, content, content.size(), fileType);
}

bool writeFile(const stringImpl& filePath, const vectorImpl<Byte>& content, size_t length, FileType fileType) {
    return writeFile(filePath, content.data(), length, fileType);
}

bool writeFile(const stringImpl& filePath, const vectorImplFast<U8>& content, FileType fileType) {
    return writeFile(filePath, reinterpret_cast<const char*>(content.data()), fileType);
}

bool writeFile(const stringImpl& filePath, const vectorImplFast<U8>& content, size_t length, FileType fileType) {
    return writeFile(filePath, reinterpret_cast<const char*>(content.data()), length, fileType);
}

bool writeFile(const stringImpl& filePath, const vectorImplFast<Byte>& content, FileType fileType) {
    return writeFile(filePath, content.data(), fileType);
}

bool writeFile(const stringImpl& filePath, const vectorImplFast<Byte>& content, size_t length, FileType fileType) {
    return writeFile(filePath, content.data(), length, fileType);
}

FileWithPath splitPathToNameAndLocation(const stringImpl& input) {
    size_t pathNameSplitPoint = input.find_last_of('/') + 1;
    return FileWithPath{
        input.substr(pathNameSplitPoint, stringImpl::npos),
        input.substr(0, pathNameSplitPoint)
    };
}

bool fileExists(const char* filePath) {
    return std::ifstream(filePath).good();
}

bool createFile(const char* filePath, bool overwriteExisting) {
    if (overwriteExisting && fileExists(filePath)) {
        return std::ifstream(filePath, std::fstream::in | std::fstream::trunc).good();
    }

    createDirectories(splitPathToNameAndLocation(filePath)._path.c_str());

    return std::ifstream(filePath, std::fstream::in).good();

}

bool hasExtension(const stringImpl& filePath, const stringImpl& extension) {
    stringImpl ext("." + extension);
    return Util::CompareIgnoreCase(Util::GetTrailingCharacters(filePath, ext.length()), ext);
}

}; //namespace Divide
