// With huge thanks to:
//    Mikko Mononen http://groups.google.com/group/recastnavigation
// And thanks to  Daniel Buckmaster, 2011 for his
// T3D 1.1 implementation of Recast's DebugUtils.

#include "Headers/NavMeshDebugDraw.h"
#include "Headers/NavMesh.h"

#include "core.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

namespace Navigation {
    NavMeshDebugDraw::NavMeshDebugDraw() : _overrideColor(false), _dirty(true), _paused(false), _color(0), _primitive(NULL)
    {
        //Generate a render state
        RenderStateBlockDescriptor navigationDebugDesc;
        navigationDebugDesc.setCullMode(CULL_MODE_NONE);
		navigationDebugDesc.setBlend(true, BLEND_PROPERTY_SRC_ALPHA, BLEND_PROPERTY_INV_SRC_ALPHA);
        _navMeshStateBlock = GFX_DEVICE.createStateBlock(navigationDebugDesc);
    }

   NavMeshDebugDraw::~NavMeshDebugDraw()
   {
       SAFE_DELETE(_navMeshStateBlock);
       //Allow the primitive to be deleted
       if(_primitive)
	       _primitive->_canZombify = true;
   }

   void NavMeshDebugDraw::paused(bool state) {
       _paused = state;
       if(_primitive)
           _primitive->paused(_paused);
   }

   void NavMeshDebugDraw::depthMask(bool state){
      _navMeshStateBlock->getDescriptor().setZReadWrite(true,state);
   }

   void NavMeshDebugDraw::texture(bool state){
   }

   void NavMeshDebugDraw::prepareMaterial(){
        SET_STATE_BLOCK(_navMeshStateBlock, true);
        GFX_DEVICE.pushWorldMatrix(mat4<F32>(), true);
   }

   void NavMeshDebugDraw::releaseMaterial(){
       GFX_DEVICE.popWorldMatrix();
   }

   void NavMeshDebugDraw::beginBatch(){
      if(!_primitive){
          _dirty = true;
          _primitive = GFX_DEVICE.createPrimitive(false);
          _primitive->setRenderStates(DELEGATE_BIND(&NavMeshDebugDraw::prepareMaterial,this),
                                      DELEGATE_BIND(&NavMeshDebugDraw::releaseMaterial,this));
      }

	  assert(_primitive != NULL);

      if(_dirty) _primitive->beginBatch();
   }

   void NavMeshDebugDraw::endBatch(){
      if(_dirty) {
          if(_primitive)
			  _primitive->endBatch();
          _dirty = false;
      }
   }

   void NavMeshDebugDraw::begin(duDebugDrawPrimitives prim, F32 size){
        if(!_dirty || !_primitive) return;

        switch(prim){
            default :
            case DU_DRAW_TRIS:   _primType = TRIANGLES;  break;
            case DU_DRAW_POINTS: _primType = API_POINTS; break;
            case DU_DRAW_LINES:  _primType = LINES;      break;
            case DU_DRAW_QUADS:  /*_primType = QUADS;*/ assert(prim == DU_DRAW_QUADS);
        }

        _primitive->attribute4ub("inColorData",vec4<U8>(255,255,255,64));
        _primitive->begin(_primType);
   }

   void NavMeshDebugDraw::vertex(const F32* pos, U32 color){
      vertex(pos[0], pos[1], pos[2], color);
   }

   void NavMeshDebugDraw::vertex(const F32 x, const F32 y, const F32 z, U32 color){
       if(!_dirty || !_primitive) return;
       if(_overrideColor)  color = _color;

       vec4<U8> colorVec;
       vec3<F32> vert(x,y,z);
       rcCol(color, colorVec.r, colorVec.g, colorVec.b, colorVec.a);
       colorVec.a = 64;

       _primitive->attribute4ub("inColorData",colorVec);
       _primitive->vertex(vert);
   }

   void NavMeshDebugDraw::vertex(const F32* pos, U32 color, const F32* uv) {
      vertex(pos[0], pos[1], pos[2], color);
   }

   void NavMeshDebugDraw::vertex(const F32 x, const F32 y, const F32 z, U32 color, const F32 u, const F32 v) {
      vertex(x, y, z, color);
   }

   void NavMeshDebugDraw::end() {
      if(_dirty && _primitive) _primitive->end();
   }

   void NavMeshDebugDraw::overrideColor(U32 col) {
      _overrideColor = true;
      _color = col;
   }

   void NavMeshDebugDraw::cancelOverride() {
      _overrideColor = false;
   }
};