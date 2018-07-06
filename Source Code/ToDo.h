/*
Copyright (c) 2016 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _DIVIDE_TO_DO_H_
#define _DIVIDE_TO_DO_H_

/*
==================== Change scene system ===============================================
Always load a default scene that will never be unloaded
- Default scene will handle both  main menu and scene transition screen
-- Add a button for each DETECTED scene
-- Create a list of needed assets for each scene. Flag common assets to avoid unloading when switching scenes
-- Never unload shaders from ShaderManager.
-- Show animated Loading text when scene button is pressed. Hide scene buttons. Load scene in separate thread. Switch active scene on load finish
-- Add quite scene dialog (Yes/No). Switch to default scene on Yes.
Remove SceneManager. Default scene will handle loading/unloading/reloading of scenes
Move scene load from XML / save to XML to the scene code (Scene::loadFromXML / Scene::saveToXML)
- saveToXML will run on a timer in PROFILE and DEBUG code only and check every n seconds/minutes if anything changed (e.g. added/deleted nodes)
Node XML description must support parent/child specification + more physics properties

==================== Change physics system =============================================
Per Object3D collision shape:
- Sphere,
- Box,
- Capsule (humanoid meshes)
- ConvexHull / Convex TriangleMesh based on triangle count / precision needed?
- BvhTriangleMeshShape for large static elements?
- HeightfieldTerrainShape
- Cylinder / Cone / Multisphere not used?
- Investigate CompoundShape
- Cache meshes to file

Per Scene
- Batch all static MESHES into a single, large, BvhTriangleMesh
- Interface physics with scenegraph for adding / removing actors

Sync PhysicsComponent with PhysicsAsset similar to this: https://www.raywenderlich.com/53077/bullet-physics-tutorial-getting-started

==================== General engine bugs / missing features ===============================
- Particles flicker
- Re-test and fix environment objects
-- Test and fix terrain
--- Apply detail via the HW tessellator
-- Test and fix vegetation
--- Improve vegetation culling: switch from GS + XFBK to Compute based system
-- Test and fix water rendering

- Finish PostFX system:
-- HDR + MSAA resolved input -> LDR output after tonemap
-- FXAA & SMAA
-- Depth of Field
--- No motion blur (bleah)
-- Luminance + ToneMap + Automatic adaptive exposure
-- LUT / Color correction

- Finish octree implementation
-- Test collisions with first person weapon
--- Rotate weapon towards body on collision.
--- Use weapon as camera collision proxy
-- Add camera collision
-- Add motion sway for first person view

- Fix shadowing system
-- Fix cubemap based omni light shadow mapping
-- Fix spot light shadow mapping
-- Fix CSM plane split seam

- Improve MSAA
-- Resolve after "end()" call. Use as PostFX input

==================== Main game initial code / setup ===================================

- Create test scene:
-- 2 sponza atriums on large terrain, opossite sides of map
-- Random NPC navigation from on building to the other
-- Each NPC has small particle emitter
-- Start position inside one building
--- No initial lights -> Holster weapon -> Flashlight turns on -> Fire weapon -> ~512 point lights turn on -> walk outside -> View entire map -> Be amazed

- Improve test scene with Physically based shading

*/
#endif //_DIVIDE_TO_DO_H_
