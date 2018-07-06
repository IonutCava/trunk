#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Importer/Headers/MeshImporter.h"

namespace Divide {

template<>
Mesh* ImplResourceLoader<Mesh>::operator()() {
    MeshImporter& importer = MeshImporter::instance();

    Mesh* ptr = nullptr;
    Import::ImportData tempMeshData;
    if (importer.loadMeshDataFromFile(_descriptor.getResourceLocation(), tempMeshData)) {
        ptr = importer.loadMesh(tempMeshData);
    } else {
        //handle error
        assert(false);
    }

    if (!load(ptr, _descriptor.getName()) || !ptr) {
        MemoryManager::DELETE(ptr);
    } else {
        if (_descriptor.getFlag()) {
            ptr->renderState().useDefaultMaterial(false);
        }
        ptr->setResourceLocation(_descriptor.getResourceLocation());
    }

    return ptr;
}

};
