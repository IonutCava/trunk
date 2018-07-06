/*
Copyright (c) 2017 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _TERRAIN_LOADER_H_
#define _TERRAIN_LOADER_H_

#include <memory>

namespace Divide {

class Terrain;
class TerrainDescriptor;
class TerrainLoader : private NonCopyable {
   public:
    static bool loadTerrain(std::shared_ptr<Terrain> terrain,
                            const std::shared_ptr<TerrainDescriptor>& terrainDescriptor,
                            PlatformContext& context,
                            const DELEGATE_CBK<void, Resource_ptr>& onLoadCallback);

   protected:
    static bool Save(const char* fileName);
    static bool Load(const char* filename);

   private:
    TerrainLoader() {}
    ~TerrainLoader() {}

    static bool loadThreadedResources(std::shared_ptr<Terrain> terrain,
                                      const std::shared_ptr<TerrainDescriptor> terrainDescriptor,
                                      DELEGATE_CBK<void, Resource_ptr> onLoadCallback);
    static void initializeVegetation(std::shared_ptr<Terrain> terrain,
                                     const std::shared_ptr<TerrainDescriptor> terrainDescriptor);
};

};  // namespace Divide

#endif