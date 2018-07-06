/*
** GLIM - OpenGL Immediate Mode
** Copyright Jan Krassnigg (Jan@Krassnigg.de)
** For more details, see the included Readme.txt.
*/

#include "glim.h"
#include <iostream>
#include <stdio.h>

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

namespace NS_GLIM
{
	using namespace std;

	GLIM_CALLBACK GLIM_Interface::s_StateChangeCallback = NULL;
	bool GLIM_Interface::s_bForceWireframe = false;

#ifdef AE_RENDERAPI_D3D11
		ID3D11Device* GLIM_Interface::s_pDevice = NULL;
		ID3D11DeviceContext* GLIM_Interface::s_pContext = NULL;
		GLIM_CALLBACK_SETINPUTLAYOUT GLIM_Interface::s_SetInputLayoutCallback = NULL;
		GLIM_CALLBACK_RELEASERESOURCE GLIM_Interface::s_ReleaseResourceCallback = NULL;
		char GLIM_Interface::s_VertexPosSemanticName[32] = "POSITION";
#endif


	glimException::glimException (const string &err) : runtime_error (err)
	{
		printf (err.c_str ());
		cerr << err;

#ifdef WIN32
		MessageBox (NULL, err.c_str (), "GLIM - Error", MB_ICONERROR);
#endif
	}



}