#include "Headers/Patch.h"
#include "Headers/XMLParser.h"

namespace Divide {

void Patch::addGeometry(const FileData& data)
{
    ModelData.push_back(data);
}

bool Patch::compareData(const PatchData& data)
{
    bool updated = true;
    XML::loadScene(data.sceneName);
    for(vectorImpl<FileData>::iterator _iter = ModelData.begin(); _iter != ModelData.end(); _iter++)
        for(U32 i = 0; i < data.size; i++)
        {
            if(data.name[i] == (*_iter).ItemName) // for each item in the scene
                if((*_iter).version != data.version[i]) // if the version differs
                {
                    if((*_iter).ModelName == data.name[i])
                        (*_iter).ModelName == "nullptr"; //Don't update modelNames

                    updated = false;
                    continue;
                }
                else     ModelData.erase(_iter);
        }
    //After the 2 for's ModelData and VegetationData contain all the geometry that needs patching;
    return updated;
}

const vectorImpl<FileData>& Patch::updateClient()
{
    return ModelData;
}

}; //namespace Divide