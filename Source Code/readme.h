/*
   Copyright (c) 2017 DIVIDE-Studio
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

 /*
ToDo:
-Finish PostFX system :
--HDR + MSAA resolved input -> LDR output after tonemap
-- FXAA & SMAA
-- Depth of Field
-- - No motion blur(bleah)
--Luminance + ToneMap + Automatic adaptive exposure
-- LUT / Colour correction

- Improve MSAA
-- Resolve after "end()" call.Use as PostFX input

- Finish octree implementation
-- Test collisions with first person weapon
-- - Rotate weapon towards body on collision.
-- - Use weapon as camera collision proxy
-- Add camera collision
-- Add motion sway for first person view

- Fix shadowing system
-- Fix cubemap based omni light shadow mapping
-- Fix spot light shadow mapping
-- Fix CSM plane split seam

- Re - test and fix environment objects
-- Test and fix terrain
-- - Apply detail via the HW tessellator
-- Test and fix vegetation
-- - Improve vegetation culling : switch from GS + XFBK to Compute based system
-- Test and fix water rendering

- Create test scene :
-- 2 Sponza atriums on large terrain, opposite sides of map
-- Random NPC navigation from on building to the other
-- Each NPC has small particle emitter
-- Start position inside one building
-- - No initial lights->Holster weapon->Flashlight turns on->Fire weapon -> ~512 point lights turn on->walk outside->View entire map->Be amazed

- Implement Physically based shading with GUIEditor controls
*/
