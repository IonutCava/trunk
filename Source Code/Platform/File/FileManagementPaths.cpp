#include "stdafx.h"

#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {

namespace Paths {
    //special case
    stringImpl g_logPath = "logs/";

    stringImpl g_exePath;
    stringImpl g_assetsLocation;
    stringImpl g_shadersLocation;
    stringImpl g_texturesLocation;
    stringImpl g_heightmapLocation;
    stringImpl g_climatesLocation;
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
    stringImpl g_cacheLocation;
    stringImpl g_terrainCacheLocation;
    stringImpl g_geometryCacheLocation;

    namespace Editor {
        stringImpl g_saveLocation;
        stringImpl g_tabLayoutFile;
        stringImpl g_panelLayoutFile;
    };

    namespace Scripts {
        stringImpl g_scriptsLocation;
        stringImpl g_scriptsAtomsLocation;
    };

    namespace Textures {
        stringImpl g_metadataLocation;
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
    std::regex g_definePattern;
    std::regex g_usePattern;

    void initPaths(const SysInfo& info) {
        g_exePath = stringImpl(info._pathAndFilename._path + "/");
        g_assetsLocation = stringImpl("assets/");
        g_shadersLocation = stringImpl("shaders/");
        g_texturesLocation = stringImpl("textures/");
        g_heightmapLocation = stringImpl("terrain/");
        g_climatesLocation = stringImpl("climates/");
        g_xmlDataLocation = stringImpl("XML/");
        g_scenesLocation = stringImpl("Scenes/");

        g_saveLocation = stringImpl("SaveData/");
        g_imagesLocation = stringImpl("misc_images/");
        g_materialsLocation = stringImpl("materials/");
        g_navMeshesLocation = stringImpl("navMeshes/");
        g_GUILocation = stringImpl("GUI/");
        g_fontsPath = stringImpl("fonts/");
        g_soundsLocation = stringImpl("sounds/");
        g_localisationPath = stringImpl("localisation/");
        g_cacheLocation = stringImpl("cache/");
        g_terrainCacheLocation = stringImpl("terrain/");
        g_geometryCacheLocation = stringImpl("geometry/");

        Scripts::g_scriptsLocation = stringImpl(g_assetsLocation + "scripts/");
        Scripts::g_scriptsAtomsLocation = stringImpl(Scripts::g_scriptsLocation + "atoms/");

        Textures::g_metadataLocation = stringImpl("textureData/");

        Editor::g_saveLocation = stringImpl("Editor/");
        Editor::g_tabLayoutFile = stringImpl("Tabs.layout");
        Editor::g_panelLayoutFile = stringImpl("Panels.layout");

        Shaders::g_cacheLocation = stringImpl("shaders/");
        Shaders::g_cacheLocationText = stringImpl(Shaders::g_cacheLocation + "Text/");
        Shaders::g_cacheLocationBin = stringImpl(Shaders::g_cacheLocation + "Binary/");
        // these must match the last 4 characters of the atom file
        Shaders::GLSL::g_fragAtomExt = stringImpl("frag");
        Shaders::GLSL::g_vertAtomExt = stringImpl("vert");
        Shaders::GLSL::g_geomAtomExt = stringImpl("geom");
        Shaders::GLSL::g_tescAtomExt = stringImpl("tesc");
        Shaders::GLSL::g_teseAtomExt = stringImpl("tese");
        Shaders::GLSL::g_compAtomExt = stringImpl(".comp");
        Shaders::GLSL::g_comnAtomExt = stringImpl(".cmn");

        Shaders::GLSL::g_parentShaderLoc = stringImpl("GLSL/");
        Shaders::GLSL::g_fragAtomLoc = stringImpl("fragmentAtoms/");
        Shaders::GLSL::g_vertAtomLoc = stringImpl("vertexAtoms/");
        Shaders::GLSL::g_geomAtomLoc = stringImpl("geometryAtoms/");
        Shaders::GLSL::g_tescAtomLoc = stringImpl("tessellationCAtoms/");
        Shaders::GLSL::g_teseAtomLoc = stringImpl("tessellationEAtoms/");
        Shaders::GLSL::g_compAtomLoc = stringImpl("computeAtoms/");
        Shaders::GLSL::g_comnAtomLoc = stringImpl("common/");

        Shaders::HLSL::g_parentShaderLoc = stringImpl("HLSL/");

        g_includePattern = std::regex(R"(^\s*#\s*include\s+["<]([^">]+)*[">])");
        g_definePattern = std::regex(R"(([#!][A-z]{2,}[\s]{1,}?([A-z]{2,}[\s]{1,}?)?)([\\(]?[^\s\\)]{1,}[\\)]?)?)");
        g_usePattern = std::regex(R"(^\s*use\s*\(\s*\"(.*)\"\s*\))");
    }

    void updatePaths(const PlatformContext& context) {
        const Configuration& config = context.config();
        const XMLEntryData& entryData = context.entryData();
        
        g_assetsLocation = stringImpl(entryData.assetsLocation + "/");
        g_shadersLocation = stringImpl(config.defaultShadersLocation + "/");
        g_texturesLocation = stringImpl(config.defaultTextureLocation + "/");
        g_xmlDataLocation = stringImpl(entryData.scriptLocation + "/");
        g_scenesLocation = stringImpl(entryData.scenesLocation + "/");
        Scripts::g_scriptsLocation = stringImpl(g_assetsLocation + "scripts/");
        Scripts::g_scriptsAtomsLocation = stringImpl(Scripts::g_scriptsLocation + "atoms/");
    }

}; //namespace Paths

}; //namespace Divide