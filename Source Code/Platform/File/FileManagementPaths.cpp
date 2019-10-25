#include "stdafx.h"

#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {

namespace Paths {
    //special case
    Str256 g_logPath = "logs/";

    Str256 g_exePath;
    Str256 g_assetsLocation;
    Str256 g_shadersLocation;
    Str256 g_texturesLocation;
    Str256 g_heightmapLocation;
    Str256 g_climatesLowResLocation;
    Str256 g_climatesMedResLocation;
    Str256 g_climatesHighResLocation;
    Str256 g_imagesLocation;
    Str256 g_materialsLocation;
    Str256 g_soundsLocation;
    Str256 g_xmlDataLocation;
    Str256 g_navMeshesLocation;
    Str256 g_scenesLocation;
    Str256 g_saveLocation;
    Str256 g_GUILocation;
    Str256 g_fontsPath;
    Str256 g_localisationPath;
    Str256 g_cacheLocation;
    Str256 g_terrainCacheLocation;
    Str256 g_geometryCacheLocation;

    namespace Editor {
        Str256 g_saveLocation;
        Str256 g_tabLayoutFile;
        Str256 g_panelLayoutFile;
    };

    namespace Scripts {
        Str256 g_scriptsLocation;
        Str256 g_scriptsAtomsLocation;
    };

    namespace Textures {
        Str256 g_metadataLocation;
    };

    namespace Shaders {
        Str256 g_cacheLocation;
        Str256 g_cacheLocationText;
        Str256 g_cacheLocationBin;

        namespace GLSL {
            // these must match the last 4 characters of the atom file
            Str8 g_fragAtomExt;
            Str8 g_vertAtomExt;
            Str8 g_geomAtomExt;
            Str8 g_tescAtomExt;
            Str8 g_teseAtomExt;
            Str8 g_compAtomExt;
            Str8 g_comnAtomExt;

            // Shader subfolder name that contains shader files for OpenGL
            Str256 g_parentShaderLoc;
            // Atom folder names in parent shader folder
            Str256 g_fragAtomLoc;
            Str256 g_vertAtomLoc;
            Str256 g_geomAtomLoc;
            Str256 g_tescAtomLoc;
            Str256 g_teseAtomLoc;
            Str256 g_compAtomLoc;
            Str256 g_comnAtomLoc;
        };

        namespace HLSL {
            Str256 g_parentShaderLoc;
        };
    };

    std::regex g_includePattern;
    std::regex g_definePattern;
    std::regex g_usePattern;

    void initPaths(const SysInfo& info) {
        g_exePath = (info._pathAndFilename._path + "/").c_str();
        g_assetsLocation = ("assets/");
        g_shadersLocation = ("shaders/");
        g_texturesLocation = ("textures/");
        g_heightmapLocation = ("terrain/");
        g_climatesLowResLocation = ("climates_05k/");
        g_climatesMedResLocation = ("climates_1k/");
        g_climatesHighResLocation = ("climates_4k/");
        g_xmlDataLocation = ("XML/");
        g_scenesLocation = ("Scenes/");

        g_saveLocation = ("SaveData/");
        g_imagesLocation = ("misc_images/");
        g_materialsLocation = ("materials/");
        g_navMeshesLocation = ("navMeshes/");
        g_GUILocation = ("GUI/");
        g_fontsPath = ("fonts/");
        g_soundsLocation = ("sounds/");
        g_localisationPath = ("localisation/");
        g_cacheLocation = ("cache/");
        g_terrainCacheLocation = ("terrain/");
        g_geometryCacheLocation = ("geometry/");

        Scripts::g_scriptsLocation = (g_assetsLocation + "scripts/");
        Scripts::g_scriptsAtomsLocation = (Scripts::g_scriptsLocation + "atoms/");

        Textures::g_metadataLocation = ("textureData/");

        Editor::g_saveLocation = ("Editor/");
        Editor::g_tabLayoutFile = ("Tabs.layout");
        Editor::g_panelLayoutFile = ("Panels.layout");

        Shaders::g_cacheLocation = ("shaders/");
        Shaders::g_cacheLocationText = (Shaders::g_cacheLocation + "Text/");
        Shaders::g_cacheLocationBin = (Shaders::g_cacheLocation + "Binary/");
        // these must match the last 4 characters of the atom file
        Shaders::GLSL::g_fragAtomExt = ("frag");
        Shaders::GLSL::g_vertAtomExt = ("vert");
        Shaders::GLSL::g_geomAtomExt = ("geom");
        Shaders::GLSL::g_tescAtomExt = ("tesc");
        Shaders::GLSL::g_teseAtomExt = ("tese");
        Shaders::GLSL::g_compAtomExt = (".comp");
        Shaders::GLSL::g_comnAtomExt = (".cmn");

        Shaders::GLSL::g_parentShaderLoc = ("GLSL/");
        Shaders::GLSL::g_fragAtomLoc = ("fragmentAtoms/");
        Shaders::GLSL::g_vertAtomLoc = ("vertexAtoms/");
        Shaders::GLSL::g_geomAtomLoc = ("geometryAtoms/");
        Shaders::GLSL::g_tescAtomLoc = ("tessellationCAtoms/");
        Shaders::GLSL::g_teseAtomLoc = ("tessellationEAtoms/");
        Shaders::GLSL::g_compAtomLoc = ("computeAtoms/");
        Shaders::GLSL::g_comnAtomLoc = ("common/");

        Shaders::HLSL::g_parentShaderLoc = ("HLSL/");

        g_includePattern = std::regex(R"(^\s*#\s*include\s+["<]([^">]+)*[">])");
        g_definePattern = std::regex(R"(([#!][A-z]{2,}[\s]{1,}?([A-z]{2,}[\s]{1,}?)?)([\\(]?[^\s\\)]{1,}[\\)]?)?)");
        g_usePattern = std::regex(R"(^\s*use\s*\(\s*\"(.*)\"\s*\))");
    }

    void updatePaths(const PlatformContext& context) {
        const Configuration& config = context.config();
        const XMLEntryData& entryData = context.entryData();
        
        g_assetsLocation = (entryData.assetsLocation + "/").c_str();
        g_shadersLocation = (config.defaultShadersLocation + "/").c_str();
        g_texturesLocation = (config.defaultTextureLocation + "/").c_str();
        g_xmlDataLocation = (entryData.scriptLocation + "/").c_str();
        g_scenesLocation = (entryData.scenesLocation + "/").c_str();
        Scripts::g_scriptsLocation = (g_assetsLocation + "scripts/");
        Scripts::g_scriptsAtomsLocation = (Scripts::g_scriptsLocation + "atoms/");
    }

}; //namespace Paths

}; //namespace Divide