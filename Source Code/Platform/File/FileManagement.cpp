#include "Headers/FileManagement.h"

#include "Core/Headers/StringHelper.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {

namespace Paths {
    const char* g_assetsLocation = nullptr;
    const char* g_shadersLocation = nullptr;
    const char* g_texturesLocation = nullptr;
    const char* g_soundsLocation = nullptr;
    const char* g_xmlDataLocation = nullptr;
    const char* g_scenesLocation = nullptr;
    const char* g_GUILocation = nullptr;
    const char* g_FontsPath = nullptr;

    const char* Shaders::g_CacheLocation = nullptr;
    const char* Shaders::g_CacheLocationText = nullptr;
    const char* Shaders::g_CacheLocationBin = nullptr;
    const char* Shaders::GLSL::g_fragAtomExt = nullptr;
    const char* Shaders::GLSL::g_vertAtomExt = nullptr;
    const char* Shaders::GLSL::g_geomAtomExt = nullptr;
    const char* Shaders::GLSL::g_tescAtomExt = nullptr;
    const char* Shaders::GLSL::g_teseAtomExt = nullptr;
    const char* Shaders::GLSL::g_compAtomExt = nullptr;
    const char* Shaders::GLSL::g_comnAtomExt = nullptr;
    const char* Shaders::GLSL::g_parentShaderLoc = nullptr;
    const char* Shaders::GLSL::g_fragAtomLoc = nullptr;
    const char* Shaders::GLSL::g_vertAtomLoc = nullptr;
    const char* Shaders::GLSL::g_geomAtomLoc = nullptr;
    const char* Shaders::GLSL::g_tescAtomLoc = nullptr;
    const char* Shaders::GLSL::g_teseAtomLoc = nullptr;
    const char* Shaders::GLSL::g_compAtomLoc = nullptr;
    const char* Shaders::GLSL::g_comnAtomLoc = nullptr;
    std::regex g_includePattern;

    void updatePaths(const PlatformContext& context) {
        const Configuration& config = context.config();
        const XMLEntryData& entryData = context.entryData();
        
        g_assetsLocation = entryData.assetsLocation.c_str();
        g_shadersLocation = config.defaultShadersLocation.c_str();
        g_texturesLocation = config.defaultTextureLocation.c_str();
        g_xmlDataLocation = entryData.scriptLocation.c_str();
        g_scenesLocation = entryData.scenesLocation.c_str();

        g_GUILocation = "GUI";
        g_FontsPath = "fonts";
        g_soundsLocation = "sounds";

        Shaders::g_CacheLocation = "shaderCache";
        Shaders::g_CacheLocationText = Util::StringFormat("%s/Text/", Shaders::g_CacheLocation).c_str();
        Shaders::g_CacheLocationBin = Util::StringFormat("%s/Binary/", Shaders::g_CacheLocation).c_str();
        // these must match the last 4 characters of the atom file
        Shaders::GLSL::g_fragAtomExt = "frag";
        Shaders::GLSL::g_vertAtomExt = "vert";
        Shaders::GLSL::g_geomAtomExt = "geom";
        Shaders::GLSL::g_tescAtomExt = "tesc";
        Shaders::GLSL::g_teseAtomExt = "tese";
        Shaders::GLSL::g_compAtomExt = ".cpt";
        Shaders::GLSL::g_comnAtomExt = ".cmn";
        Shaders::GLSL::g_parentShaderLoc = "GLSL";
        Shaders::GLSL::g_fragAtomLoc = "fragmentAtoms";
        Shaders::GLSL::g_vertAtomLoc = "vertexAtoms";
        Shaders::GLSL::g_geomAtomLoc = "geometryAtoms";
        Shaders::GLSL::g_tescAtomLoc = "tessellationCAtoms";
        Shaders::GLSL::g_teseAtomLoc = "tessellationEAtoms";
        Shaders::GLSL::g_compAtomLoc = "computeAtoms";
        Shaders::GLSL::g_comnAtomLoc = "common";

        g_includePattern = std::regex("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
    }
};

void ReadTextFile(const stringImpl& filePath, stringImpl& contentOut) {
    std::ifstream inFile(filePath.c_str(), std::ios::in);

    if (!inFile.eof() && !inFile.fail())
    {
        assert(inFile.good());
        inFile.seekg(0, std::ios::end);
        contentOut.reserve(inFile.tellg());
        inFile.seekg(0, std::ios::beg);

        contentOut.assign((std::istreambuf_iterator<char>(inFile)),
                           std::istreambuf_iterator<char>());
    }

    inFile.close();
}

stringImpl ReadTextFile(const stringImpl& filePath) {
    stringImpl content;
    ReadTextFile(filePath, content);
    return content;
}

void WriteTextFile(const stringImpl& filePath, const stringImpl& content) {
    if (filePath.empty()) {
        return;
    }
    std::ofstream outputFile(filePath.c_str(), std::ios::out);
    outputFile << content;
    outputFile.close();
    assert(outputFile.good());
}

std::pair<stringImpl/*fileName*/, stringImpl/*filePath*/>
SplitPathToNameAndLocation(const stringImpl& input) {
    size_t pathNameSplitPoint = input.find_last_of('/') + 1;

    return std::make_pair(input.substr(pathNameSplitPoint + 1, stringImpl::npos),
                          input.substr(0, pathNameSplitPoint));
}

bool FileExists(const char* filePath) {
    return std::ifstream(filePath).good();
}

}; //namespace Divide