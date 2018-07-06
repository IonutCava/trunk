#include "Headers/Scene.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Managers/Headers/SceneManager.h"

void Scene::onLostFocus(){
   state()._moveFB = 0;
   state()._moveLR = 0;
   state()._roll   = 0; 
   state()._angleLR = 0;
   state()._angleUD = 0;
#ifndef _DEBUG
   _paramHandler.setParam("freezeLoopTime", true);
#endif
}

namespace {
    struct selectionQueueDistanceFrontToBack {
        selectionQueueDistanceFrontToBack(const vec3<F32>& eyePos) : _eyePos(eyePos) {}

        bool operator()(SceneGraphNode* const a, SceneGraphNode* const b) const {
            F32 dist_a = a->getBoundingBoxConst().nearestDistanceFromPointSquared(_eyePos);
            F32 dist_b = b->getBoundingBoxConst().nearestDistanceFromPointSquared(_eyePos);
            return dist_a > dist_b;
        }
    private:
        vec3<F32> _eyePos;
    };
    static vectorImpl<SceneGraphNode* > _sceneSelectionCandidates;
}

void Scene::findSelection(F32 mouseX, F32 mouseY){
    mouseY = renderState().cachedResolution().height - mouseY - 1;
    vec3<F32> startRay = renderState().getCameraConst().unProject(vec3<F32>(mouseX, mouseY, 0.0f));
    vec3<F32> endRay   = renderState().getCameraConst().unProject(vec3<F32>(mouseX, mouseY, 1.0f));
    const vec2<F32>& zPlanes = renderState().getCameraConst().getZPlanes();

    // deselect old node
    if(_currentSelection) _currentSelection->setSelected(false);
    _currentSelection = nullptr;

    // see if we select another one
    _sceneSelectionCandidates.clear();
    // Cast the picking ray and find items between the nearPlane (with a small offset) and limit the range to half of the far plane
    _sceneGraph->Intersect(Ray(startRay, startRay.direction(endRay)), zPlanes.x + 0.5f, zPlanes.y * 0.5f, _sceneSelectionCandidates); 
    if(!_sceneSelectionCandidates.empty()){
        std::sort(_sceneSelectionCandidates.begin(), _sceneSelectionCandidates.end(), selectionQueueDistanceFrontToBack(renderState().getCameraConst().getEye()));
        _currentSelection = _sceneSelectionCandidates[0];
        // set it's state to selected
        _currentSelection->setSelected(true);
#ifdef _DEBUG
        _pointsA[DEBUG_LINE_RAY_PICK].push_back(startRay);
        _pointsB[DEBUG_LINE_RAY_PICK].push_back(endRay);
        _colors[DEBUG_LINE_RAY_PICK].push_back(vec4<U8>(0,255,0,255));
#endif
    }
}

bool Scene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
    _mousePressed[button] = true;
    switch (button){
        case OIS::MB_Left:   break;
        case OIS::MB_Right:   break;
        case OIS::MB_Middle:  break;
        case OIS::MB_Button3: break;
        case OIS::MB_Button4: break;
        case OIS::MB_Button5: break;
        case OIS::MB_Button6: break;
        case OIS::MB_Button7: break;
    }
    return true;
}

bool Scene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
    _mousePressed[button] = false;
    switch (button){
        case OIS::MB_Left:    findSelection(key.state.X.abs, key.state.Y.abs); break;
        case OIS::MB_Right:   break;
        case OIS::MB_Middle:  break;
        case OIS::MB_Button3: break;
        case OIS::MB_Button4: break;
        case OIS::MB_Button5: break;
        case OIS::MB_Button6: break;
        case OIS::MB_Button7: break;
    }
    return true;
}
  
bool Scene::onMouseMove(const OIS::MouseEvent& key){ 
    _previousMousePos.set(key.state.X.abs, key.state.Y.abs);
    return true;
}

void Scene::defaultCameraKeys(OIS::KeyCode code, bool upState){

    if (upState){
        switch (code){
            case OIS::KC_W: if (state()._moveFB == 1) state()._moveFB = 0; break;
            case OIS::KC_S: if (state()._moveFB == -1) state()._moveFB = 0; break;
            case OIS::KC_A: if (state()._moveLR == -1) state()._moveLR = 0; break;
            case OIS::KC_D:	if (state()._moveLR == 1) state()._moveLR = 0; break;
            case OIS::KC_Q: if (state()._roll == -1)   state()._roll = 0;   break;
            case OIS::KC_E: if (state()._roll == 1)   state()._roll = 0;   break;
            case OIS::KC_LEFT: if (state()._angleLR == -1) state()._angleLR = 0; break;
            case OIS::KC_RIGHT: if (state()._angleLR == 1) state()._angleLR = 0; break;
            case OIS::KC_UP: if (state()._angleUD == -1) state()._angleUD = 0; break;
            case OIS::KC_DOWN: if (state()._angleUD == 1) state()._angleUD = 0; break;
        }
    }else{
        switch (code){
            case OIS::KC_W: state()._moveFB =  1; break;
            case OIS::KC_S: state()._moveFB = -1; break;
            case OIS::KC_A: state()._moveLR = -1; break;
            case OIS::KC_D: state()._moveLR =  1; break;
            case OIS::KC_Q: state()._roll = -1;   break;
            case OIS::KC_E: state()._roll =  1;   break;
            case OIS::KC_LEFT:  state()._angleLR = -1; break;
            case OIS::KC_RIGHT: state()._angleLR =  1; break;
            case OIS::KC_UP:    state()._angleUD = -1; break;
            case OIS::KC_DOWN:  state()._angleUD =  1; break;
        }
    }
}

bool Scene::onKeyDown(const OIS::KeyEvent& key){
    defaultCameraKeys(key.key, false);

    switch(key.key){
        default: break;
        case OIS::KC_END   : deleteSelection(); break;
        case OIS::KC_ADD   : {
            Camera& cam = renderState().getCamera();
            F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
            if (currentCamMoveSpeedFactor < 50){
                cam.setMoveSpeedFactor( currentCamMoveSpeedFactor + 1.0f);
                cam.setTurnSpeedFactor( cam.getTurnSpeedFactor()  + 1.0f);
            }
        }break;
        case OIS::KC_SUBTRACT :	{
            Camera& cam = renderState().getCamera();
            F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
            if (currentCamMoveSpeedFactor > 1.0f){
                cam.setMoveSpeedFactor( currentCamMoveSpeedFactor - 1.0f);
                cam.setTurnSpeedFactor( cam.getTurnSpeedFactor() - 1.0f);
            }
        }break;
    }

    return true;
}

bool Scene::onKeyUp(const OIS::KeyEvent& key){
    defaultCameraKeys(key.key, true);

    switch( key.key ){
        case OIS::KC_P: 
            _paramHandler.setParam("freezeLoopTime", !_paramHandler.getParam("freezeLoopTime", false)); 
            break;
        case OIS::KC_F2:
            renderState().toggleSkeletons();
            break;
        case OIS::KC_F3:
            _paramHandler.setParam("postProcessing.enableDepthOfField", !_paramHandler.getParam<bool>("postProcessing.enableDepthOfField"));
            break;
        case OIS::KC_F4:
            _paramHandler.setParam("postProcessing.enableBloom", !_paramHandler.getParam<bool>("postProcessing.enableBloom"));
            break;
        case OIS::KC_F5:
            GFX_DEVICE.drawDebugAxis(!GFX_DEVICE.drawDebugAxis());
            break;
        case OIS::KC_F6: {
            static bool state = true;
            PostFX::getInstance().toggleDepthPreview(state);
            _paramHandler.setParam("postProcessing.fullScreenDepthBuffer", state);
            state = !state;
        }break;
        case OIS::KC_B:{
            renderState().toggleBoundingBoxes();
            }break;
        case OIS::KC_F7:
            GFX_DEVICE.postProcessingEnabled(!GFX_DEVICE.postProcessingEnabled());
            break;
        case OIS::KC_F8:
            renderState().drawDebugLines(!renderState()._debugDrawLines);
            break;
        case OIS::KC_F9:{
#ifdef _DEBUG
            for(U8 i = 0; i < DEBUG_LINE_PLACEHOLDER; ++i) {
                _pointsA[i].clear();
                _pointsB[i].clear();
                _colors[i].clear();
            }
#endif
            }break;
        case OIS::KC_F10:
            LightManager::getInstance().togglePreviewShadowMaps();
            GFX_DEVICE.togglePreviewDepthBuffer();
            break;
        case OIS::KC_F12:
            GFX_DEVICE.Screenshot("screenshot_");
            break;
        default:
            break;
    }

    return true;
}

bool Scene::onJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis,I32 deadZone){
    if(key.device->getID() != InputInterface::JOY_1) return false;

    I32 axisABS = key.state.mAxes[axis].abs;

    if(axis == 1){
        if(axisABS > deadZone)   	 state()._angleLR = 1;
        else if(axisABS < -deadZone) state()._angleLR = -1;
        else 			             state()._angleLR = 0;
    }else if(axis == 0){
        if(axisABS > deadZone)       state()._angleUD = 1;
        else if(axisABS < -deadZone) state()._angleUD = -1;
        else 			             state()._angleUD = 0;
    }else if(axis == 2){
        if(axisABS < -deadZone)  	 state()._moveFB = 1;
        else if(axisABS > deadZone)  state()._moveFB = -1;
        else			             state()._moveFB = 0;
    }else if(axis == 3){
        if(axisABS < -deadZone)      state()._moveLR = -1;
        else if(axisABS > deadZone)  state()._moveLR = 1;
        else                         state()._moveLR = 0;
    }
    return true;
}

bool Scene::onJoystickMovePOV(const OIS::JoyStickEvent& key, I8 pov){
    if (key.state.mPOV[pov].direction & OIS::Pov::North) //Going up
        state()._moveFB = 1;
    else if (key.state.mPOV[pov].direction & OIS::Pov::South) //Going down
        state()._moveFB = -1;

    if (key.state.mPOV[pov].direction & OIS::Pov::East) //Going right
        state()._moveLR = 1;

    else if (key.state.mPOV[pov].direction & OIS::Pov::West) //Going left
        state()._moveLR = -1;

    if (key.state.mPOV[pov].direction == OIS::Pov::Centered){ //stopped/centered out
        state()._moveLR = 0;
        state()._moveFB = 0;
    }
    return true;
}