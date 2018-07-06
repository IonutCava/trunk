#include "Headers/Scene.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Managers/Headers/SceneManager.h"

struct selectionQueueDistanceFrontToBack{
    bool operator()(SceneGraphNode* const a, SceneGraphNode* const b) const {
        const vec3<F32>& eye = Frustum::getInstance().getEyePos();
        F32 dist_a = a->getBoundingBoxConst().nearestDistanceFromPointSquared(eye);
        F32 dist_b = b->getBoundingBoxConst().nearestDistanceFromPointSquared(eye);
        return dist_a > dist_b;
    }
};

static vectorImpl<SceneGraphNode* > _sceneSelectionCandidates;
void Scene::findSelection(F32 mouseX, F32 mouseY){
    const vec2<F32>& zPlanes = Frustum::getInstance().getZPlanes();
    mouseY = renderState().cachedResolution().height - mouseY - 1;
    vec3<F32> startRay = GFX_DEVICE.unproject(vec3<F32>(mouseX, mouseY, 0.0f));
    vec3<F32> endRay   = GFX_DEVICE.unproject(vec3<F32>(mouseX, mouseY, 1.0f));

    // deselect old node
    if(_currentSelection) _currentSelection->setSelected(false);
    // see if we select another one
    _sceneSelectionCandidates.clear();
    // Cast the picking ray and find items between the nearPlane (with a small offset) and limit the range to half of the far plane
    _sceneGraph->Intersect(Ray(startRay, startRay.direction(endRay)), zPlanes.x + 0.5f, zPlanes.y * 0.5f, _sceneSelectionCandidates); 
    if(!_sceneSelectionCandidates.empty()){
        std::sort(_sceneSelectionCandidates.begin(), _sceneSelectionCandidates.end(), selectionQueueDistanceFrontToBack());
        _currentSelection = _sceneSelectionCandidates[0];
    }else{
        _currentSelection = nullptr;
    }
     // set it's state to selected
    if(_currentSelection) _currentSelection->setSelected(true);
#ifdef _DEBUG
    _pointsA[DEBUG_LINE_RAY_PICK].push_back(startRay);
    _pointsB[DEBUG_LINE_RAY_PICK].push_back(endRay);
    _colors[DEBUG_LINE_RAY_PICK].push_back(vec4<U8>(0,255,0,255));
#endif
}

bool Scene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
    _mousePressed[button] = true;
    return true;
}

bool Scene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
    _mousePressed[button] = false;
    if(button == OIS::MB_Left){
        findSelection(key.state.X.abs, key.state.Y.abs);
    }
    return true;
}
  
bool Scene::onMouseMove(const OIS::MouseEvent& key){ 
    _previousMousePos.set(key.state.X.abs, key.state.Y.abs);
    return true;
}

bool Scene::onKeyDown(const OIS::KeyEvent& key){
    switch(key.key){
        default: break;
        case OIS::KC_LEFT  : state()._angleLR = -1; break;
        case OIS::KC_RIGHT : state()._angleLR =  1; break;
        case OIS::KC_UP    : state()._angleUD = -1; break;
        case OIS::KC_DOWN  : state()._angleUD =  1; break;
        case OIS::KC_END   : deleteSelection(); break;
        case OIS::KC_ADD   : {
            Camera& cam = renderState().getCamera();
            F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
            if (currentCamMoveSpeedFactor < 5){
                cam.setMoveSpeedFactor( currentCamMoveSpeedFactor + 0.1f);
                cam.setTurnSpeedFactor( cam.getTurnSpeedFactor() + 0.1f);
            }
        }break;
        case OIS::KC_SUBTRACT :	{
            Camera& cam = renderState().getCamera();
            F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
            if (currentCamMoveSpeedFactor > 0.2f){
                cam.setMoveSpeedFactor( currentCamMoveSpeedFactor - 0.1f);
                cam.setTurnSpeedFactor( cam.getTurnSpeedFactor() - 0.1f);
            }
        }break;
    }

    return true;
}

bool Scene::onKeyUp(const OIS::KeyEvent& key){
    switch( key.key ){
        case OIS::KC_LEFT :
        case OIS::KC_RIGHT:	state()._angleLR = 0; break;
        case OIS::KC_UP   :
        case OIS::KC_DOWN : state()._angleUD = 0; break;
        case OIS::KC_F2:{
            renderState().toggleSkeletons();
        }break;
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
            SceneManager::getInstance().togglePreviewDepthBuffer();
            break;
        case OIS::KC_F12:
            GFX_DEVICE.Screenshot("screenshot_",vec4<F32>(0,0,renderState()._cachedResolution.x,renderState()._cachedResolution.y));
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