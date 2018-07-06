#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Importer/Headers/MeshImporter.h"

namespace Divide {

template<>
Resource_ptr ImplResourceLoader<Mesh>::operator()() {
    MeshImporter& importer = MeshImporter::instance();

    Mesh_ptr ptr;
    Import::ImportData tempMeshData(_descriptor.getResourceLocation());
    if (importer.loadMeshDataFromFile(_context, _cache, tempMeshData)) {
        ptr = importer.loadMesh(_context, _cache, _descriptor.getName(), tempMeshData);
    } else {
        //handle error
        DIVIDE_UNEXPECTED_CALL(Util::StringFormat("Failed to import mesh [ %s ]!",
                               _descriptor.getResourceLocation()).c_str());
    }

    if (!load(ptr) || !ptr) {
        ptr.reset();
    } else {
        if (_descriptor.getFlag()) {
            ptr->renderState().useDefaultMaterial(false);
        }
    }

    return ptr;
}

};
