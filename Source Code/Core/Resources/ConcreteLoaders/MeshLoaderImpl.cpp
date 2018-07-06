#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Importer/Headers/MeshImporter.h"

namespace Divide {

template<>
Mesh* ImplResourceLoader<Mesh>::operator()() {
    MeshImporter& importer = MeshImporter::instance();

    Mesh* ptr = nullptr;
    Import::ImportData tempMeshData(_descriptor.getResourceLocation());
    if (importer.loadMeshDataFromFile(tempMeshData)) {
        ptr = importer.loadMesh(_descriptor.getName(), tempMeshData);
    } else {
        //handle error
        assert(false);
    }

    if (!load(ptr) || !ptr) {
        MemoryManager::DELETE(ptr);
    } else {
        if (_descriptor.getFlag()) {
            ptr->renderState().useDefaultMaterial(false);
        }
    }

    return ptr;
}

};
