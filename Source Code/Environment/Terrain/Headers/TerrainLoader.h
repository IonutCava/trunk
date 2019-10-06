/*
Copyright (c) 2018 DIVIDE-Studio
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

namespace Divide {

enum class TerrainTextureType : U8 {
    ALBEDO_ROUGHNESS = 0,
    NORMAL,
    DISPLACEMENT_AO,
    COUNT
};

class TerrainDescriptor;
FWD_DECLARE_MANAGED_CLASS(Terrain);
class TerrainLoader : private NonCopyable {
   public:
    static bool loadTerrain(Terrain_ptr terrain,
                            const std::shared_ptr<TerrainDescriptor>& terrainDescriptor,
                            PlatformContext& context,
                            bool threadedLoading);



   protected:
    static bool Save(const char* fileName);
    static bool Load(const char* filename);

   private:
    TerrainLoader() noexcept {}
    ~TerrainLoader() {}

    static bool loadThreadedResources(Terrain_ptr terrain,
                                     PlatformContext& context,
                                      const std::shared_ptr<TerrainDescriptor> terrainDescriptor);

    static void initializeVegetation(Terrain_ptr terrain,
                                     PlatformContext& context,
                                     const std::shared_ptr<TerrainDescriptor> terrainDescriptor);
};

};  // namespace Divide

#endif