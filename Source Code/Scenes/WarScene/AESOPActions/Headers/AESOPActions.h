/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */
#ifndef _WAR_SCENE_AESOP_ACTION_INTERFACE_H_
#define _WAR_SCENE_AESOP_ACTION_INTERFACE_H_

#include "AI/Headers/AESOPActionInterface.h"

namespace AI {
    // Some basic locations
    const Aesop::PVal mapEastNorth    = 'A';
    const Aesop::PVal mapEastMiddle   = 'B';
    const Aesop::PVal mapEastSouth    = 'C';
    const Aesop::PVal mapCenterNorth  = 'D';
    const Aesop::PVal mapCenterMiddle = 'E';
    const Aesop::PVal mapCenterSouth  = 'F';
    const Aesop::PVal mapWestNorth    = 'G';
    const Aesop::PVal mapWestMiddle   = 'H';
    const Aesop::PVal mapWestSouth    = 'I';
    // Some useful predicates
    enum {
        At,
        Life,
        Friends,
        Enemies,
        Adjacent
    };
    class MoveAction : public AESOPAction {
        public:
            MoveAction(std::string name, F32 cost = 1.0f);
    };
    class RunAction : public AESOPAction {
        public:
            RunAction(std::string name, F32 cost = 1.0f);
    };
    class KillAction : public AESOPAction {
        public:
            KillAction(std::string name, F32 cost = 1.0f);
    };
    class CapturePointAction : public AESOPAction {
        public:
            CapturePointAction(std::string name, F32 cost = 1.0f);
    };
    class DieAction : public AESOPAction {
        public:
            DieAction(std::string name, F32 cost = 1.0f);
    };
};
#endif