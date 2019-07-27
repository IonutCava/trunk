#include "stdafx.h"

#include "Headers/TerrainDescriptor.h"

#include "Core/Headers/StringHelper.h"
#include "Platform/File/Headers/FileManagement.h"

namespace Divide {
    TerrainDescriptor::TerrainDescriptor(const stringImpl& name) noexcept
        : PropertyDescriptor(PropertyDescriptor::DescriptorType::DESCRIPTOR_TERRAIN_INFO)
    {
    }

    TerrainDescriptor::~TerrainDescriptor()
    {
        _variables.clear();
    }

    bool TerrainDescriptor::loadFromXML(const boost::property_tree::ptree& pt, const stringImpl& name) {
        const stringImpl terrainDescriptor = pt.get<stringImpl>("descriptor", "");
        if (terrainDescriptor.empty()) {
            return false;
        }
        
        addVariable("terrainName", name.c_str());
        addVariable("descriptor", terrainDescriptor.c_str());
        setWireframeDebug(pt.get<I32>("wireframeDebugMode", 0));
        addVariable("waterCaustics", pt.get<stringImpl>("waterCaustics"));
        addVariable("underwaterAlbedoTexture", pt.get<stringImpl>("underwaterAlbedoTexture", "sandfloor009a.jpg"));
        addVariable("underwaterDetailTexture", pt.get<stringImpl>("underwaterDetailTexture", "terrain_detail_NM.png"));
        addVariable("tileNoiseTexture", pt.get<stringImpl>("tileNoiseTexture", "bruit_gaussien_2.jpg"));
        addVariable("underwaterTileScale", pt.get<F32>("underwaterTileScale"));
        stringImpl alphaMapDescriptor = "";
        {
            boost::property_tree::ptree descTree = {};
            read_xml(Paths::g_assetsLocation + Paths::g_heightmapLocation + terrainDescriptor + "/descriptor.xml", descTree);

            addVariable("heightfield", descTree.get<stringImpl>("heightfield", ""));
            addVariable("heightfieldTex", descTree.get<stringImpl>("heightfieldTex", ""));
            addVariable("horizontalScale", descTree.get<F32>("horizontalScale", 1.0f));
            addVariable("albedoTilingFactor", descTree.get<F32>("albedoTilingFactor", 4.0f));
            addVariable("detailTilingFactor", descTree.get<F32>("detailTilingFactor", 4.0f));
            addVariable("detailBrightnessFactor", descTree.get<F32>("detailBrightnessFactor", 1.8f));
            addVariable("normalMap", descTree.get<stringImpl>("normalMap", ""));
            addVariable("textureMap", descTree.get<stringImpl>("textureMap", ""));
            setDimensions(vec2<U16>(descTree.get<U16>("heightfieldResolution.<xmlattr>.x", 0), descTree.get<U16>("heightfieldResolution.<xmlattr>.y", 0)));
            setAltitudeRange(vec2<F32>(descTree.get<F32>("altitudeRange.<xmlattr>.min", 0.0f), descTree.get<F32>("altitudeRange.<xmlattr>.max", 255.0f)));
            setTessellationRange(vec4<F32>(descTree.get<F32>("tessellationRange.<xmlattr>.min", 10.0f),
                                           descTree.get<F32>("tessellationRange.<xmlattr>.max", 150.0f),
                                           descTree.get<F32>("tessellationRange.<xmlattr>.chunkSize", 32.0f),
                                           descTree.get<F32>("tessellationRange.<xmlattr>.patchSizeInM", 100.0f)));
            addVariable("vegetationTextureLocation", descTree.get<stringImpl>("vegetation.vegetationTextureLocation", Paths::g_imagesLocation));
            addVariable("grassMap", descTree.get<stringImpl>("vegetation.grassMap"));
            addVariable("treeMap", descTree.get<stringImpl>("vegetation.treeMap"));

            for (I32 j = 1; j < 5; ++j) {
                addVariable(Util::StringFormat("grassBillboard%d", j), descTree.get<stringImpl>(Util::StringFormat("vegetation.grassBillboard%d", j), ""));
                addVariable(Util::StringFormat("grassScale%d", j), descTree.get<F32>(Util::StringFormat("vegetation.grassBillboard%d.<xmlattr>.scale", j), 1.0f));

                addVariable(Util::StringFormat("treeMesh%d", j), descTree.get<stringImpl>(Util::StringFormat("vegetation.treeMesh%d", j), ""));
                addVariable(Util::StringFormat("treeScale%d", j), descTree.get<F32>(Util::StringFormat("vegetation.treeMesh%d.<xmlattr>.scale", j), 1.0f));
                addVariable(Util::StringFormat("treeRotationX%d", j), descTree.get<F32>(Util::StringFormat("vegetation.treeMesh%d.<xmlattr>.rotate_x", j), 0.0f));
                addVariable(Util::StringFormat("treeRotationY%d", j), descTree.get<F32>(Util::StringFormat("vegetation.treeMesh%d.<xmlattr>.rotate_y", j), 0.0f));
                addVariable(Util::StringFormat("treeRotationZ%d", j), descTree.get<F32>(Util::StringFormat("vegetation.treeMesh%d.<xmlattr>.rotate_z", j), 0.0f));
            }
            alphaMapDescriptor = descTree.get<stringImpl>("alphaMaps", "");
        }

        if (alphaMapDescriptor.empty()) {
            return false;
        }
        {
            boost::property_tree::ptree alphaTree = {};
            read_xml(Paths::g_assetsLocation + Paths::g_heightmapLocation + terrainDescriptor + "/" + alphaMapDescriptor, alphaTree);

            const U8 numLayers = alphaTree.get<U8>("AlphaData.nImages");
            const U8 numImages = alphaTree.get<U8>("AlphaData.nLayers");
            if (numLayers == 0 || numImages == 0) {
                return false;
            }
            setTextureLayerCount(numLayers);

            const stringImpl imageListNode = "AlphaData.ImageList";
            I32 i = 0;
            stringImpl blendMap;
            std::array<stringImpl, 4> arrayMaterials;
            stringImpl layerOffsetStr;
            for (boost::property_tree::ptree::iterator itImage = std::begin(alphaTree.get_child(imageListNode));
                itImage != std::end(alphaTree.get_child(imageListNode));
                ++itImage, ++i)
            {
                stringImpl layerName(itImage->second.data());
                stringImpl format(itImage->first.data());

                if (format.find("<xmlcomment>") != stringImpl::npos) {
                    i--;
                    continue;
                }

                layerOffsetStr = to_stringImpl(i);
                addVariable("blendMap" + layerOffsetStr, stripQuotes(itImage->second.get<stringImpl>("FileName", "")));

                for (boost::property_tree::ptree::iterator itLayer = std::begin(itImage->second.get_child("LayerList"));
                    itLayer != std::end(itImage->second.get_child("LayerList"));
                    ++itLayer)
                {
                    if (stringImpl(itLayer->first.data()).find("<xmlcomment>") != stringImpl::npos) {
                        continue;
                    }

                    stringImpl layerColour = itLayer->second.get<stringImpl>("LayerColour", "");
                    stringImpl materialName = "";
                    stringImpl selectedTexture = "";
                    for (boost::property_tree::ptree::iterator itTextures = std::begin(itLayer->second.get_child("TexList"));
                        itTextures != std::end(itLayer->second.get_child("TexList"));
                        ++itTextures)
                    {
                        if (stringImpl(itTextures->first.data()).find("<xmlcomment>") != stringImpl::npos) {
                            continue;
                        }

                        selectedTexture = splitPathToNameAndLocation(stripQuotes(itTextures->second.data()))._fileName;
                        // Only one material per channel = one texture per channel!
                        break;
                    }

                    for (boost::property_tree::ptree::iterator itMaterial = std::begin(itLayer->second.get_child("MtlList"));
                        itMaterial != std::end(itLayer->second.get_child("MtlList"));
                        ++itMaterial)
                    {
                        if (stringImpl(itMaterial->first.data()).find("<xmlcomment>") != stringImpl::npos) {
                            continue;
                        }

                        materialName = itMaterial->second.data();
                        // Only one material per channel!
                        break;
                    }

                    addVariable(layerColour + layerOffsetStr + "_mat", materialName);
                    addVariable(layerColour + layerOffsetStr + "_tex", selectedTexture);
                }
            }
        }

        return true;
    }

    void TerrainDescriptor::clone(std::shared_ptr<PropertyDescriptor>& target) const {
        target.reset(new TerrainDescriptor(*this));
    }

    void TerrainDescriptor::addVariable(const stringImpl& name, const stringImpl& value) {
        hashAlg::insert(_variables, hashAlg::make_pair(_ID(name.c_str()), value));
    }

    void TerrainDescriptor::addVariable(const stringImpl& name, F32 value) {
        hashAlg::insert(_variablesf, hashAlg::make_pair(_ID(name.c_str()), value));
    }
}; //namespace Divide