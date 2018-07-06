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

        g_imagesLocation = "misc_images/";
        g_materialsLocation = "materials/";
        g_navMeshesLocation = "navMeshes/";
        g_GUILocation = "GUI/";
        g_FontsPath = "fonts/";
        g_soundsLocation = "sounds/";

        Shaders::g_CacheLocation = "shaderCache/";
        Shaders::g_CacheLocationText = Util::StringFormat("%s/Text/", Shaders::g_CacheLocation);
        Shaders::g_CacheLocationBin = Util::StringFormat("%s/Binary/", Shaders::g_CacheLocation);
        // these must match the last 4 characters of the atom file
        Shaders::GLSL::g_fragAtomExt = "frag";
        Shaders::GLSL::g_vertAtomExt = "vert";
        Shaders::GLSL::g_geomAtomExt = "geom";
        Shaders::GLSL::g_tescAtomExt = "tesc";
        Shaders::GLSL::g_teseAtomExt = "tese";
        Shaders::GLSL::g_compAtomExt = ".cpt";
        Shaders::GLSL::g_comnAtomExt = ".cmn";
        Shaders::GLSL::g_parentShaderLoc = "GLSL";
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

    return std::make_pair(input.substr(pathNameSplitPoint, stringImpl::npos),
                          input.substr(0, pathNameSplitPoint));
}

bool FileExists(const char* filePath) {
    return std::ifstream(filePath).good();
}

bool HasExtension(const stringImpl& filePath, const stringImpl& extension) {
    stringImpl ext("." + extension);
    return Util::CompareIgnoreCase(Util::GetTrailingCharacters(filePath, ext.length()), ext);
}

}; //namespace Divide