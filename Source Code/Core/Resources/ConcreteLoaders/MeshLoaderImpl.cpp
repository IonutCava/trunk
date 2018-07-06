#include "Core/Headers/StringHelper.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Importer/Headers/MeshImporter.h"

namespace Divide {

struct MeshLoadData {
    explicit MeshLoadData(Mesh_ptr mesh,
                          ResourceCache* cache,
                          PlatformContext* context,
                          ResourceDescriptor descriptor)
        : _mesh(mesh),
          _cache(cache),
          _context(context),
          _descriptor(descriptor)
    {
    }

    Mesh_ptr _mesh;
    ResourceCache* _cache;
    PlatformContext* _context;
    ResourceDescriptor _descriptor;

};

void threadedMeshLoad(MeshLoadData loadData, Import::ImportData tempMeshData) {
    if (MeshImporter::loadMeshDataFromFile(*loadData._context, *loadData._cache, tempMeshData)) {
        if (!MeshImporter::loadMesh(loadData._mesh, *loadData._context, *loadData._cache, tempMeshData)) {
            loadData._mesh.reset();
        }
    } else {
        loadData._mesh.reset();
        //handle error
        DIVIDE_UNEXPECTED_CALL(Util::StringFormat("Failed to import mesh [ %s ]!", loadData._descriptor.getResourceLocation().c_str()).c_str());
    }

    if (!loadData._mesh->load(loadData._descriptor.onLoadCallback())) {
        loadData._mesh.reset();
    } else {
        if (loadData._descriptor.getFlag()) {
            loadData._mesh->renderState().useDefaultMaterial(false);
        }
    }
}

template<>
CachedResource_ptr ImplResourceLoader<Mesh>::operator()() {
    Import::ImportData tempMeshData(_descriptor.getResourceLocation());
    Mesh_ptr ptr(MemoryManager_NEW Mesh(_context.gfx(),
                                        _cache,
                                        _loadingDescriptorHash,
                                        _descriptor.getName(),
                                        tempMeshData._modelName,
                                        tempMeshData._modelPath),
                                DeleteResource(_cache));

    if (ptr) {
        ptr->setState(ResourceState::RES_LOADING);
    }

    MeshLoadData loadingData(ptr, &_cache, &_context, _descriptor);
    _context.gfx().loadInContext(_descriptor.getThreaded() ? CurrentContext::GFX_LOADING_CTX : CurrentContext::GFX_RENDERING_CTX,
    [this, tempMeshData, loadingData](const Task& parent) {
        threadedMeshLoad(loadingData, tempMeshData);
    });


    return ptr;
}

};
