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

#ifndef _GENERIC_DRAW_COMMAND_INL_
#define _GENERIC_DRAW_COMMAND_INL_

namespace Divide {

    inline bool isEnabledOption(const GenericDrawCommand& cmd, CmdRenderOptions option) {
        return BitCompare(cmd._renderOptions, to_U32(option));
    }

    inline void enableOption(GenericDrawCommand& cmd, CmdRenderOptions option) {
        SetBit(cmd._renderOptions, to_U32(option));
    }

    inline void disableOption(GenericDrawCommand& cmd, CmdRenderOptions option) {
        ClearBit(cmd._renderOptions, to_U32(option));
    }

    inline void toggleOption(GenericDrawCommand& cmd, CmdRenderOptions option) {
        setOption(cmd, option, !isEnabledOption(cmd, option));
    }

    inline void setOption(GenericDrawCommand& cmd, CmdRenderOptions option, const bool state) {
        if (state) {
            enableOption(cmd, option);
        } else {
            disableOption(cmd, option);
        }
    }

    inline void enableOptions(GenericDrawCommand& cmd, U32 optionsMask) {
        SetBit(cmd._renderOptions, to_U32(optionsMask));
    }

    inline void disableOptions(GenericDrawCommand& cmd, U32 optionsMask) {
        ClearBit(cmd._renderOptions, to_U32(optionsMask));
    }

}; //namespace Divide
#endif //_GENERIC_DRAW_COMMAND_INL