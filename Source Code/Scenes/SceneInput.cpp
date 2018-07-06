#include "Headers/Scene.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"

void Scene::findSelection(const vec3<F32>& camOrigin, U32 x, U32 y){
    F32 value_fov = tan(RADIANS(_paramHandler.getParam<F32>("runtime.verticalFOV")) * 0.5f);
    F32 value_aspect = _paramHandler.getParam<F32>("runtime.aspectRatio");
    F32 half_resolution_width = renderState()._cachedResolution.width * 0.5f;
    F32 half_resolution_height = renderState()._cachedResolution.height * 0.5f;

    //mathematical handling of the difference between
    //your mouse position and the 'center' of the window
    F32 modifier_x, modifier_y;

     F32 point_dist = _paramHandler.getParam<F32>("runtime.zFar");
        //it'll be put this far on the Z plane

    vec3<F32> camera_origin;
        //this is where the camera sits, in 3dspace

    vec3<F32> point_xformed;
        //this is the transformed point

    vec3<F32> final_point;
    vec4<F32> color(0.0, 1.0, 0.0, 1.0);

    //These lines are the biggest part of this function.
    //This is where the mouse position is turned into a mathematical
    //'relative' of 3d space. The conversion to an actual point
    modifier_x = value_fov * (( 1.0f - x / half_resolution_width ) * ( value_aspect ) );
    modifier_y = value_fov * -( 1.0f - y / half_resolution_height );

    //These 3 take our modifier_x/y values and our 'casting' distance
    //to throw out a point in space that lies on the point_dist plane.
    //If we were using completely untransformed, untranslated space,
    //this would be fine - but we're not :)
    //the untransformed ray will be put here
    vec3<F32> point(modifier_x * point_dist, modifier_y * point_dist, point_dist);

    //Next we make a call to grab our MODELVIEW_MATRIX -
    //This is the matrix that rasters 3d points to 2d space - which is
    //kinda what we're doing, in reverse
    mat4<GLfloat> temp;
    GFX_DEVICE.getMatrix(VIEW_MATRIX,temp);
    //Some folks would then invert the matrix - I invert the results.

    //First, to get the camera_origin, we transform the 12, 13, 14
    //slots of our pulledMatrix - this gets us the actual viewing
    //position we are 'sitting' at when the function is called
    camera_origin.x = -(
        temp.mat[0] * temp.mat[12] +
        temp.mat[1] * temp.mat[13] +
        temp.mat[2] * temp.mat[14]);
    camera_origin.y = -(
        temp.mat[4] * temp.mat[12] +
        temp.mat[5] * temp.mat[13] +
        temp.mat[6] * temp.mat[14]);
    camera_origin.z = -(
        temp.mat[8] * temp.mat[12] +
        temp.mat[9] * temp.mat[13] +
        temp.mat[10] * temp.mat[14]);

    //Second, we transform the position we generated earlier - the '3d'
    //mouse position - by our viewing matrix.
    point_xformed.x = -(
        temp.mat[0] * point[0] +
        temp.mat[1] * point[1] +
        temp.mat[2] * point[2]);
    point_xformed.y = -(
        temp.mat[4] * point[0] +
        temp.mat[5] * point[1] +
        temp.mat[6] * point[2]);
    point_xformed.z = -(
        temp.mat[8] * point[0] +
        temp.mat[9] * point[1] +
        temp.mat[10] * point[2]);

    final_point = point_xformed + camera_origin;

    vec3<F32> origin(camOrigin);
    vec3<F32> dir = origin.direction(final_point);

    Ray r(origin,dir);
    SceneGraphNode* previousSelection = _currentSelection;
    _currentSelection = _sceneGraph->Intersect(r,Frustum::getInstance().getZPlanes().x,
                                                 Frustum::getInstance().getZPlanes().y/2.0f);
    if(previousSelection) previousSelection->setSelected(false);
    if(_currentSelection) _currentSelection->setSelected(true);
}

void Scene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
    _mousePressed[button] = true;
}

void Scene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
    _mousePressed[button] = false;
}

void Scene::onKeyDown(const OIS::KeyEvent& key){
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
        case OIS::KC_F2:{
            D_PRINT_FN(Locale::get("TOGGLE_SCENE_SKELETONS"));
            renderState().toggleSkeletons();
            }break;
        case OIS::KC_B:{
            D_PRINT_FN(Locale::get("TOGGLE_SCENE_BOUNDING_BOXES"));
            renderState().toggleBoundingBoxes();
            }break;
        case OIS::KC_F8:
            _paramHandler.setParam("postProcessing.enablePostFX",!_paramHandler.getParam<bool>("postProcessing.enablePostFX"));
            break;
        case OIS::KC_F10:
            LightManager::getInstance().togglePreviewShadowMaps();
            break;
        case OIS::KC_F12:
            GFX_DEVICE.Screenshot("screenshot_",vec4<F32>(0,0,renderState()._cachedResolution.x,renderState()._cachedResolution.y));
            break;
    }
}

void Scene::onKeyUp(const OIS::KeyEvent& key){
    switch( key.key ){
        case OIS::KC_LEFT :
        case OIS::KC_RIGHT:	state()._angleLR = 0; break;
        case OIS::KC_UP   :
        case OIS::KC_DOWN : state()._angleUD = 0; break;
        case OIS::KC_F3:
            _paramHandler.setParam("postProcessing.enableDepthOfField", !_paramHandler.getParam<bool>("postProcessing.enableDepthOfField"));
            break;
        case OIS::KC_F4:
            _paramHandler.setParam("postProcessing.enableBloom", !_paramHandler.getParam<bool>("postProcessing.enableBloom"));
            break;
        case OIS::KC_F5:
            GFX_DEVICE.drawDebugAxis(!GFX_DEVICE.drawDebugAxis());
            break;
        default:
            break;
    }
}

void Scene::onJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis,I32 deadZone){
    if(key.device->getID() != InputInterface::JOY_1) return;

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
}