#include "Headers/FileManagement.h"

#include "Core/Headers/StringHelper.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {

namespace Paths {

    stringImplAligned g_assetsLocation;
    stringImplAligned g_shadersLocation;
    stringImplAligned g_texturesLocation;
    stringImplAligned g_imagesLocation;
    stringImplAligned g_materialsLocation;
    stringImplAligned g_soundsLocation;
    stringImplAligned g_xmlDataLocation;
    stringImplAligned g_navMeshesLocation;
    stringImplAligned g_scenesLocation;
    stringImplAligned g_saveLocation;
    stringImplAligned g_GUILocation;
    stringImplAligned g_FontsPath;

    namespace Shaders {
        stringImplAligned g_CacheLocation;
        stringImplAligned g_CacheLocationText;
        stringImplAligned g_CacheLocationBin;

        namespace GLSL {
            // these must match the last 4 characters of the atom file
            stringImplAligned g_fragAtomExt;
            stringImplAligned g_vertAtomExt;
            stringImplAligned g_geomAtomExt;
            stringImplAligned g_tescAtomExt;
            stringImplAligned g_teseAtomExt;
            stringImplAligned g_compAtomExt;
            stringImplAligned g_comnAtomExt;

            // Shader subfolder name that contains shader files for OpenGL
            stringImplAligned g_parentShaderLoc;
            // Atom folder names in parent shader folder
            stringImplAligned g_fragAtomLoc;
            stringImplAligned g_vertAtomLoc;
            stringImplAligned g_geomAtomLoc;
            stringImplAligned g_tescAtomLoc;
            stringImplAligned g_teseAtomLoc;
            stringImplAligned g_compAtomLoc;
            stringImplAligned g_comnAtomLoc;
        };
    };
    std::regex g_includePattern;

    void updatePaths(const PlatformContext& context) {
        const Configuration& config = context.config();
        const XMLEntryData& entryData = context.entryData();
        
        g_assetsLocation = entryData.assetsLocation + "/";
        g_shadersLocation = config.defaultShadersLocation + "/";
        g_texturesLocation = config.defaultTextureLocation + "/";
        g_xmlDataLocation = entryData.scriptLocation + "/";
        g_scenesLocation = entryData.scenesLocation + "/";
        g_saveLocation = "SaveData/";
        g_imagesLocation = "misc_images/";
        g_materialsLocation = "materials/";
        g_navMeshesLocation = "navMeshes/";
        g_GUILocation = "GUI/";
        g_FontsPath = "fonts/";
        g_soundsLocation = "sounds/";

        Shaders::g_CacheLocation = "shaderCache/";
        Shaders::g_CacheLocationText = Shaders::g_CacheLocation + "Text/";
        Shaders::g_CacheLocationBin = Shaders::g_CacheLocation + "Binary/";
        // these must match the last 4 characters of the atom file
        Shaders::GLSL::g_fragAtomExt = "frag";
        Shaders::GLSL::g_vertAtomExt = "vert";
        Shaders::GLSL::g_geomAtomExt = "geom";
        Shaders::GLSL::g_tescAtomExt = "tesc";
        Shaders::GLSL::g_teseAtomExt = "tese";
        Shaders::GLSL::g_compAtomExt = ".cpt";
        Shaders::GLSL::g_comnAtomExt = ".cmn";

        Shaders::GLSL::g_parentShaderLoc = "GLSL/";
        Shaders::GLSL::g_fragAtomLoc = "fragmentAtoms/";
        Shaders::GLSL::g_vertAtomLoc = "vertexAtoms/";
        Shaders::GLSL::g_geomAtomLoc = "geometryAtoms/";
        Shaders::GLSL::g_tescAtomLoc = "tessellationCAtoms/";
        Shaders::GLSL::g_teseAtomLoc = "tessellationEAtoms/";
        Shaders::GLSL::g_compAtomLoc = "computeAtoms/";
        Shaders::GLSL::g_comnAtomLoc = "common/";

        g_includePattern = std::regex("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
    }
};

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

            std::copy_n(std::istreambuf_iterator<char>(inFile), fsize, std::back_inserter(contentOut));
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