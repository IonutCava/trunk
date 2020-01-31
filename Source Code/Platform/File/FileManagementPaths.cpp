#include "stdafx.h"

#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Headers/FileManagement.h"

namespace Divide {

Str64 Paths::g_logPath = "logs/";

Str128 Paths::g_exePath;
Str64 Paths::g_assetsLocation;
Str64 Paths::g_shadersLocation;
Str64 Paths::g_texturesLocation;
Str64 Paths::g_heightmapLocation;
Str64 Paths::g_climatesLowResLocation;
Str64 Paths::g_climatesMedResLocation;
Str64 Paths::g_climatesHighResLocation;
Str64 Paths::g_xmlDataLocation;
Str64 Paths::g_scenesLocation;

Str64 Paths::g_saveLocation;
Str64 Paths::g_imagesLocation;
Str64 Paths::g_materialsLocation;
Str64 Paths::g_navMeshesLocation;
Str64 Paths::g_GUILocation;
Str64 Paths::g_fontsPath;
Str64 Paths::g_soundsLocation;
Str64 Paths::g_localisationPath;
Str64 Paths::g_cacheLocation;
Str64 Paths::g_terrainCacheLocation;
Str64 Paths::g_geometryCacheLocation;

Str64 Paths::Editor::g_saveLocation;
Str64 Paths::Editor::g_tabLayoutFile;
Str64 Paths::Editor::g_panelLayoutFile;

Str64 Paths::Scripts::g_scriptsLocation;
Str64 Paths::Scripts::g_scriptsAtomsLocation;

Str64 Paths::Textures::g_metadataLocation;

Str64 Paths::Shaders::g_cacheLocation;
Str64 Paths::Shaders::g_cacheLocationText;
Str64 Paths::Shaders::g_cacheLocationBin;

// these must match the last 4 characters of the atom file
Str8 Paths::Shaders::GLSL::g_fragAtomExt;
Str8 Paths::Shaders::GLSL::g_vertAtomExt;
Str8 Paths::Shaders::GLSL::g_geomAtomExt;
Str8 Paths::Shaders::GLSL::g_tescAtomExt;
Str8 Paths::Shaders::GLSL::g_teseAtomExt;
Str8 Paths::Shaders::GLSL::g_compAtomExt;
Str8 Paths::Shaders::GLSL::g_comnAtomExt;

// Shader subfolder name that contains shader files for OpenGL
Str64 Paths::Shaders::GLSL::g_parentShaderLoc;
// Atom folder names in parent shader folder
Str64 Paths::Shaders::GLSL::g_fragAtomLoc;
Str64 Paths::Shaders::GLSL::g_vertAtomLoc;
Str64 Paths::Shaders::GLSL::g_geomAtomLoc;
Str64 Paths::Shaders::GLSL::g_tescAtomLoc;
Str64 Paths::Shaders::GLSL::g_teseAtomLoc;
Str64 Paths::Shaders::GLSL::g_compAtomLoc;
Str64 Paths::Shaders::GLSL::g_comnAtomLoc;

Str64 Paths::Shaders::HLSL::g_parentShaderLoc;

std::regex Paths::g_includePattern;
std::regex Paths::g_definePattern;
std::regex Paths::g_usePattern;

void Paths::initPaths(const SysInfo& info) {
    g_exePath.assign(info._pathAndFilename._path + "/");
    g_logPath = "logs/";

    g_assetsLocation = "assets/";
    g_shadersLocation = "shaders/";
    g_texturesLocation = "textures/";
    g_heightmapLocation = "terrain/";
    g_climatesLowResLocation = "climates_05k/";
    g_climatesMedResLocation = "climates_1k/";
    g_climatesHighResLocation = "climates_4k/";
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
    g_cacheLocation = "cache/";
    g_terrainCacheLocation = "terrain/";
    g_geometryCacheLocation = "geometry/";

    Editor::g_saveLocation = "Editor/";
    Editor::g_tabLayoutFile = "Tabs.layout";
    Editor::g_panelLayoutFile = "Panels.layout";

    Scripts::g_scriptsLocation = Paths::g_assetsLocation + "scripts/";
    Scripts::g_scriptsAtomsLocation = Paths::Scripts::g_scriptsLocation + "atoms/";

    Textures::g_metadataLocation = "textureData/";

    Shaders::g_cacheLocation = "shaders/";
    Shaders::g_cacheLocationText = Paths::Shaders::g_cacheLocation + "Text/";
    Shaders::g_cacheLocationBin = Paths::Shaders::g_cacheLocation + "Binary/";

    // these must match the last 4 characters of the atom file
    Shaders::GLSL::g_fragAtomExt = "frag";
    Shaders::GLSL::g_vertAtomExt = "vert";
    Shaders::GLSL::g_geomAtomExt = "geom";
    Shaders::GLSL::g_tescAtomExt = "tesc";
    Shaders::GLSL::g_teseAtomExt = "tese";
    Shaders::GLSL::g_compAtomExt = "comp";
    Shaders::GLSL::g_comnAtomExt = ".cmn";

    // Shader subfolder name that contains shader files for OpenGL
    Shaders::GLSL::g_parentShaderLoc = "GLSL/";
    // Atom folder names in parent shader folder
    Shaders::GLSL::g_fragAtomLoc = "fragmentAtoms/";
    Shaders::GLSL::g_vertAtomLoc = "vertexAtoms/";
    Shaders::GLSL::g_geomAtomLoc = "geometryAtoms/";
    Shaders::GLSL::g_tescAtomLoc = "tessellationCAtoms/";
    Shaders::GLSL::g_teseAtomLoc = "tessellationEAtoms/";
    Shaders::GLSL::g_compAtomLoc = "computeAtoms/";
    Shaders::GLSL::g_comnAtomLoc = "common/";

    Shaders::HLSL::g_parentShaderLoc = "HLSL/";

    g_includePattern = std::regex(R"(^\s*#\s*include\s+["<]([^">]+)*[">])");
    g_definePattern = std::regex(R"(([#!][A-z]{2,}[\s]{1,}?([A-z]{2,}[\s]{1,}?)?)([\\(]?[^\s\\)]{1,}[\\)]?)?)");
    g_usePattern = std::regex(R"(^\s*use\s*\(\s*\"(.*)\"\s*\))");
}

void Paths::updatePaths(const PlatformContext& context) {
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

}; //namespace Divide