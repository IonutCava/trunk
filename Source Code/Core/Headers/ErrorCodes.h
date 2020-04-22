/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#pragma once
#ifndef _CORE_ERROR_CODES_H_
#define _CORE_ERROR_CODES_H_

namespace Divide {
    enum class ErrorCode : I8 {
        NO_ERR = 0,
        MISSING_SCENE_DATA = -1,
        MISSING_SCENE_LOAD_CALL = -2,
        CPU_NOT_SUPPORTED = -3,
        GFX_NOT_SUPPORTED = -4,
        GFX_NON_SPECIFIED = -5,
        SFX_NON_SPECIFIED = -6,
        PFX_NON_SPECIFIED = -7,
        WINDOW_INIT_ERROR = -8,
        SDL_WINDOW_INIT_ERROR = -9,
        FONT_INIT_ERROR = -10,
        GLBINGING_INIT_ERROR = -11,
        GLSL_INIT_ERROR = -12,
        GL_OLD_HARDWARE = -13,
        DX_INIT_ERROR = -14,
        DX_OLD_HARDWARE = -15,
        OGL_OLD_HARDWARE = -16,
        SDL_AUDIO_INIT_ERROR = -17,
        SDL_AUDIO_MIX_INIT_ERROR = -18,
        FMOD_AUDIO_INIT_ERROR = -19,
        OAL_INIT_ERROR = -20,
        OCL_INIT_ERROR = -21,
        PHYSX_INIT_ERROR = -22,
        PHYSX_EXTENSION_ERROR = -23,
        NO_LANGUAGE_INI = -24,
        NOT_ENOUGH_RAM = -25,
        WRONG_WORKING_DIRECTORY = -26,
        PLATFORM_INIT_ERROR = -27,
        PLATFORM_CLOSE_ERROR = -28,
        EDITOR_INIT_ERROR = -29
    };
};

#endif //_CORE_ERROR_CODES_H_
