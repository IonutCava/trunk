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
void DX_API::translate(F32 x, F32 y, F32 z)
{
}

void DX_API::translate(D32 x, D32 y, D32 z)
{
}

void DX_API::rotate(F32 angle, F32 x, F32 y, F32 z)
{
}

void DX_API::rotate(D32 angle, D32 x, D32 y, D32 z)
{
}

void DX_API::scale(F32 x, F32 y, F32 z)
{
}

void DX_API::scale(D32 x, D32 y, D32 z)
{
}

void DX_API::scale(int x, int y, int z)
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
void DX_API::drawTextToScreen(Text* text)
{
}

void DX_API::drawCharacterToScreen(void* ,char)
{
}

void DX_API::drawButton(Button* button)
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