#include "Object3D.h"
#include "Managers/SceneManager.h"

void Object3D::render(SceneGraphNode* const node){
	_gfx.renderModel(node);
}
