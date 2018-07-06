#include "DXWrapper.h"
#include "Utility/Headers/DataTypes.h"
#include <iostream>
void DX_API::initHardware()
{
	std::cout << "Initializing Direct3D rendering API! " << endl;
}

void DX_API::closeRenderingApi()
{
}
void DX_API::translate(vec3& pos)
{
}

void DX_API::rotate(F32 angle, vec3& weights)
{
}


void DX_API::scale(vec3& scale)
{
}


void DX_API::clearBuffers(int buffer_mask)
{
}

void DX_API::swapBuffers()
{
}

void DX_API::enableFog(F32 density, F32* color)
{
}

void DX_API::pushMatrix()
{
}

void DX_API::popMatrix()
{
}

void DX_API::enable_MODELVIEW()
{
}

void DX_API::enable_PROJECTION()
{
}

void DX_API::enable_TEXTURE(int slot)
{
}

void DX_API::loadIdentityMatrix()
{
}

void DX_API::loadOrtographicView()
{
}

void DX_API::loadModelView()
{
}

void DX_API::toggle2D3D(bool _3D)
{
}

void DX_API::drawTextToScreen(Text* text)
{
}

void DX_API::drawCharacterToScreen(void* ,char)
{
}

void DX_API::drawButton(Button* button)
{
}

void DX_API::drawCube(F32 size)
{
}

void DX_API::drawSphere(F32 size, U32 resolution)
{
}

void DX_API::drawQuad(vec3& _topLeft, vec3& _topRight, vec3& _bottomLeft, vec3& _bottomRight)
{
}

void DX_API::renderMesh(const Mesh& mesh)
{
}

void DX_API::renderSubMesh(const SubMesh& subMesh)
{
}

void DX_API::setColor(F32 r, F32 g, F32 b)
{
}

void DX_API::setColor(D32 r, D32 g, D32 b)
{
}

void DX_API::setColor(int r, int g, int b)
{
}

void DX_API::setColor(F32 r, F32 g, F32 b, F32 alpha)
{
}

void DX_API::setColor(D32 r, D32 g, D32 b, D32 alpha)
{
}

void DX_API::setColor(int r, int g, int b, int alpha)
{
}

void DX_API::setColor(F32 *v)
{
}

void DX_API::setColor(D32 *v)
{
}

void DX_API::setColor(int *v)
{
}

void DX_API::initDevice()
{
}