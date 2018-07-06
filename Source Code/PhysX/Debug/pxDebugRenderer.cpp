#include "Hardware/Video/OpenGL/glResources.h" //ToDo: Remove this from here -Ionut

#include "pxDebugRenderer.h"
#include "NxDebugRenderable.h"
#include "resource.h"
#include "Hardware/Video/GFXDevice.h"

void DebugRenderer::renderBuffer(F32* pVertList, F32* pColorList, I32 type, I32 num)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,GL_FLOAT, 0, pVertList);
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_FLOAT, 0, pColorList);
	glDrawArrays(type, 0, num);
	glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void DebugRenderer::renderData(const NxDebugRenderable& data) const
{
	glLineWidth(1.0f);
	glDisable(GL_LIGHTING);

	U32 NbPoints = data.getNbPoints();
	if(NbPoints)
	{
		F32* pVertList = new F32[NbPoints*3];
    	F32* pColorList = new F32[NbPoints*4];
    	I32 vertIndex = 0;
    	I32 colorIndex = 0;
		const NxDebugPoint* Points = data.getPoints();
		while(NbPoints--)
		{
        	pVertList[vertIndex++] = Points->p.x;
        	pVertList[vertIndex++] = Points->p.y;
        	pVertList[vertIndex++] = Points->p.z;
        	pColorList[colorIndex++] = (F32)((Points->color>>16)&0xff)/255.0f;
        	pColorList[colorIndex++] = (F32)((Points->color>>8)&0xff)/255.0f;
        	pColorList[colorIndex++] = (F32)(Points->color&0xff)/255.0f;
        	pColorList[colorIndex++] = 1.0f;
	      	Points++;
		}
		
		renderBuffer(pVertList, pColorList, GL_POINTS, data.getNbPoints());
    	
    	delete[] pVertList;
    	delete[] pColorList;
		pVertList = NULL;
		pColorList = NULL;
	}

	U32 NbLines = data.getNbLines();
	if(NbLines)
	{
		F32* pVertList = new F32[NbLines*3*2];
    	F32* pColorList = new F32[NbLines*4*2];
    	I32 vertIndex = 0;
    	I32 colorIndex = 0;
		const NxDebugLine* Lines = data.getLines();
		while(NbLines--)
		{
        	pVertList[vertIndex++] = Lines->p0.x;
        	pVertList[vertIndex++] = Lines->p0.y;
        	pVertList[vertIndex++] = Lines->p0.z;
        	pColorList[colorIndex++] = (F32)((Lines->color>>16)&0xff)/255.0f;
        	pColorList[colorIndex++] = (F32)((Lines->color>>8)&0xff)/255.0f;
        	pColorList[colorIndex++] = (F32)(Lines->color&0xff)/255.0f;
        	pColorList[colorIndex++] = 1.0f;

        	pVertList[vertIndex++] = Lines->p1.x;
        	pVertList[vertIndex++] = Lines->p1.y;
        	pVertList[vertIndex++] = Lines->p1.z;
        	pColorList[colorIndex++] = (F32)((Lines->color>>16)&0xff)/255.0f;
        	pColorList[colorIndex++] = (F32)((Lines->color>>8)&0xff)/255.0f;
        	pColorList[colorIndex++] = (F32)(Lines->color&0xff)/255.0f;
        	pColorList[colorIndex++] = 1.0f;

	      	Lines++;
		}
		
		renderBuffer(pVertList, pColorList, GL_LINES, data.getNbLines()*2);
    	
    	delete[] pVertList;
    	delete[] pColorList;
		pVertList = NULL;
		pColorList = NULL;
	}

	U32 NbTris = data.getNbTriangles();
	if(NbTris)
	{
		F32* pVertList = new F32[NbTris*3*3];
    	F32* pColorList = new F32[NbTris*4*3];
    	I32 vertIndex = 0;
    	I32 colorIndex = 0;
		const NxDebugTriangle* Triangles = data.getTriangles();
		while(NbTris--)
		{
        	pVertList[vertIndex++] = Triangles->p0.x;
        	pVertList[vertIndex++] = Triangles->p0.y;
        	pVertList[vertIndex++] = Triangles->p0.z;

        	pVertList[vertIndex++] = Triangles->p1.x;
        	pVertList[vertIndex++] = Triangles->p1.y;
        	pVertList[vertIndex++] = Triangles->p1.z;

        	pVertList[vertIndex++] = Triangles->p2.x;
        	pVertList[vertIndex++] = Triangles->p2.y;
        	pVertList[vertIndex++] = Triangles->p2.z;

			for(I8 i=0;i<3;i++)
			{
        		pColorList[colorIndex++] = (F32)((Triangles->color>>16)&0xff)/255.0f;
        		pColorList[colorIndex++] = (F32)((Triangles->color>>8)&0xff)/255.0f;
        		pColorList[colorIndex++] = (F32)(Triangles->color&0xff)/255.0f;
        		pColorList[colorIndex++] = 1.0f;
			}

	      	Triangles++;
		}
		
		renderBuffer(pVertList, pColorList, GL_TRIANGLES, data.getNbTriangles()*3);
  	
    	delete[] pVertList;
    	delete[] pColorList;
		pVertList = NULL;
		pColorList = NULL;
	}
	glEnable(GL_LIGHTING);
}

