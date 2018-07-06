#include "Headers/FileManagement.h"

#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {

namespace Paths {

    stringImpl g_assetsLocation;
    stringImpl g_shadersLocation;
    stringImpl g_texturesLocation;
    stringImpl g_heightmapLocation;
    stringImpl g_imagesLocation;
    stringImpl g_materialsLocation;
    stringImpl g_soundsLocation;
    stringImpl g_xmlDataLocation;
    stringImpl g_navMeshesLocation;
    stringImpl g_scenesLocation;
    stringImpl g_saveLocation;
    stringImpl g_GUILocation;
    stringImpl g_fontsPath;
    stringImpl g_localisationPath;

    namespace Scripts {
        stringImpl g_scriptsLocation;
        stringImpl g_scriptsAtomsLocation;
    };

    namespace Shaders {
        stringImpl g_cacheLocation;
        stringImpl g_cacheLocationText;
        stringImpl g_cacheLocationBin;

        namespace GLSL {
            // these must match the last 4 characters of the atom file
            stringImpl g_fragAtomExt;
            stringImpl g_vertAtomExt;
            stringImpl g_geomAtomExt;
            stringImpl g_tescAtomExt;
            stringImpl g_teseAtomExt;
            stringImpl g_compAtomExt;
            stringImpl g_comnAtomExt;

            // Shader subfolder name that contains shader files for OpenGL
            stringImpl g_parentShaderLoc;
            // Atom folder names in parent shader folder
            stringImpl g_fragAtomLoc;
            stringImpl g_vertAtomLoc;
            stringImpl g_geomAtomLoc;
            stringImpl g_tescAtomLoc;
            stringImpl g_teseAtomLoc;
            stringImpl g_compAtomLoc;
            stringImpl g_comnAtomLoc;
        };

        namespace HLSL {
            stringImpl g_parentShaderLoc;
        };
    };

    std::regex g_includePattern;
    std::regex g_usePattern;

    void initPaths() {
        g_assetsLocation = "assets/";
        g_shadersLocation = "shaders/";
        g_texturesLocation = "textures/";
        g_heightmapLocation = "terrain/";
        g_xmlDataLocation = "XML/";
        g_scenesLocation = "Scenes/";

        g_saveLocation = "SaveData/";
        g_imagesLocation = "misc_images/";
        g_materialsLocation = "materials/";
        g_navMeshesLocation = "navMeshes/";
        g_GUILocation = "GUI/";
        g_fontsPath = "fonts/";
        g_soundsLocation = "sounds/";
        g_localisationPath = "localisation/";
        Scripts::g_scriptsLocation = g_assetsLocation + "scripts/";
        Scripts::g_scriptsAtomsLocation = Scripts::g_scriptsLocation + "atoms/";

        Shaders::g_cacheLocation = "shaderCache/";
        Shaders::g_cacheLocationText = Shaders::g_cacheLocation + "Text/";
        Shaders::g_cacheLocationBin = Shaders::g_cacheLocation + "Binary/";
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

        Shaders::HLSL::g_parentShaderLoc = "HLSL/";

        g_includePattern = std::regex(R"(^\s*#\s*include\s+["<]([^">]+)*[">])");
        g_usePattern = std::regex(R"(^\s*use\s*\(\s*\"(.*)\"\s*\))");
    }

    void updatePaths(const PlatformContext& context) {
        const Configuration& config = context.config();
        const XMLEntryData& entryData = context.entryData();
        
        g_assetsLocation = entryData.assetsLocation + "/";
        g_shadersLocation = config.defaultShadersLocation + "/";
        g_texturesLocation = config.defaultTextureLocation + "/";
        g_xmlDataLocation = entryData.scriptLocation + "/";
        g_scenesLocation = entryData.scenesLocation + "/";
        Scripts::g_scriptsLocation = g_assetsLocation + "scripts/";
        Scripts::g_scriptsAtomsLocation = Scripts::g_scriptsLocation + "atoms/";
    }

}; //namespace Paths

}; //namespace Divide