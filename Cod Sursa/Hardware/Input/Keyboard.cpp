#include "Utility/Headers/Guardian.h"
#include "Keyboard.h"
#include "PhysX/PhysX.h"
#include "GUI/GLUIManager.h"
#include "GUI/zpr.h"
#include "Utility/Headers/ParamHandler.h"

namespace ZPR
{
	UBYTE prevKey;
	static F32 speedFactor = 3;
	void SpecialUpKeyboard(int Key,int x, int y)
	{
		switch( Key )
		{
			case GLUT_KEY_LEFT:
			case GLUT_KEY_RIGHT:
				Engine::getInstance().angleLR = 0.0f;
				break;
			case GLUT_KEY_UP:
			case GLUT_KEY_DOWN:
				Engine::getInstance().moveFB = 0.0f;
				break;
			case GLUT_KEY_PAGE_UP:
			case GLUT_KEY_PAGE_DOWN:
				Engine::getInstance().angleUD = 0.0f;
				break;
		}
	}

	void SpecialKeyboard(int Key, int x, int y)
	{
		
			switch( Key)
			{
				case GLUT_KEY_LEFT : 
					Engine::getInstance().angleLR = -(0.01f + speedFactor/100);
					Engine::getInstance().turn -= 5.0f;
					break;
				case GLUT_KEY_RIGHT : 
					Engine::getInstance().angleLR = 0.01f + speedFactor/100;
					Engine::getInstance().turn += 5.0f;
					break;
				case GLUT_KEY_UP : 
					Engine::getInstance().moveFB = speedFactor;
					break;
				case GLUT_KEY_DOWN : 
					Engine::getInstance().moveFB = -speedFactor;
					break;
				case GLUT_KEY_PAGE_UP :
					Engine::getInstance().angleUD = -(0.007f + speedFactor/100);
					Engine::getInstance().tip  -= 5.0f;
					break;
				case GLUT_KEY_PAGE_DOWN : 
					Engine::getInstance().angleUD = 0.007f + speedFactor/100;
					Engine::getInstance().tip  += 5.0f;
					break;
			}
	}
	
	void Keyboard(UBYTE Key, int x, int y)
	{
		ParamHandler &par = ParamHandler::getInstance();
		//Acest simplu check verifica daca am apasat o alta tasta fata de cea precedenta
		//Clauza asta "if" a fost implementata pentru a evita spamul in consola daca tinem o tasta apasata
		if(prevKey != Key)
		{
			prevKey = Key;
			cout << "Received key: " << Key << endl;
		}
		switch ( Key)
		{

		case 'r':
			Guardian::getInstance().ReloadEngine();
			break;
		case 'p':
			Guardian::getInstance().RestartPhysX();
			break;
		case 'n':
			Engine::getInstance().ToggleWireframeRendering();
			break;
		case '+':
			if (speedFactor < 10)
				speedFactor += 0.1f;
			break;
		case '-':
			if (speedFactor > 0)
				speedFactor -= 0.1f;
			break;
		case 'f':
			Engine::getInstance().toggleFirstPerson();
			break;
		//1+2+3+4 = cream diversi actori prin scena
		case '1':
			glui_cb(10);
			break;
		case '2':
			glui_cb(30);
			break;
		case '3':
		    glui_cb(50);
			break;
		case '4':
			glui_cb(40);
			break;
		case '5':
			glui_cb(60);
			break;
		case '6':
			PhysX::getInstance().ApplyForceToActor(PhysX::getInstance().GetSelectedActor(), NxVec3(+1,0,0), 3000); 
			break;
		case 'z':
		    glui_cb(QUIT_ID);
			break;
		default: 
			break;
		}

	}
}

