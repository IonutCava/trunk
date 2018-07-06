#include "Headers/GUIConsole.h"
#include "Core/Headers/Application.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Managers/Headers/ResourceManager.h"
#include "Hardware/Video/RenderStateBlock.h"

GUIConsole::GUIConsole() : GUIElement() , _consoleOpen(false), _animationRunning(false), _animationDuration(750.0f) {

	_guiType = GUI_CONSOLE;
	_screenPercentage = 0.75f;

	F32 width = Application::getInstance().getWindowDimensions().width;
	F32 height = Application::getInstance().getWindowDimensions().height;
	ResourceDescriptor materialDescriptor("consoleMaterial");
	Material* mat = CreateResource<Material>(materialDescriptor);
	ResourceDescriptor rectDescriptor("consoleRect");
	rectDescriptor.setFlag(true); ///no default material
	_consoleRect = CreateResource<Quad3D>(rectDescriptor);
	///Full screen Quad and use scissor test to clip it
	_consoleRect->setDimensions(vec4<F32>(0,0,width,height));
	_consoleRect->setMaterial(mat);
	mat->setDiffuse(vec4<F32>(0.8f, 0.2f, 0.3f, 0.75f));
	RenderStateBlockDescriptor d;
	d.setCullMode(CULL_MODE_None);
	d.setZEnable(false);
	d.setBlend(true, BLEND_PROPERTY_SrcAlpha, BLEND_PROPERTY_InvSrcAlpha);
	SAFE_DELETE(_guiSB);
	_guiSB = GFX_DEVICE.createStateBlock(d);
}

void GUIConsole::setConsoleBlendFactor(F32 factor){
	CLAMP(factor,0.0f,1.0f);
	vec4<F32> oldColor = _consoleRect->getMaterial()->getMaterialMatrix().getCol(1); ///Diffuse color
	_consoleRect->getMaterial()->setDiffuse(vec4<F32>(oldColor.r,oldColor.g,oldColor.b,factor));
}

void GUIConsole::setConsoleBackgroundColor(const vec3<F32>& color){
	vec4<F32> oldColor = _consoleRect->getMaterial()->getMaterialMatrix().getCol(1); ///Diffuse color
	_consoleRect->getMaterial()->setDiffuse(vec4<F32>(color.r,color.g,color.b,oldColor.a));
}

GUIConsole::~GUIConsole() {
	RemoveResource(_consoleRect);
}

F32 GUIConsole::getConsoleHeight() {
	//determine dimensions of scissor region
	F32 height = _viewportDimensions.y, scissorWidth = _viewportDimensions.x;
	F32 currentMSTime = GETMSTIME();
	F32 currentTime = currentMSTime - _lastCheck;
	if(currentTime > (_animationDuration)){///convert to milliseconds 
		_animationRunning = false;
	}
	if(_animationRunning)	{
		F32 elapsed = GETMSTIME();
		height *= ((elapsed / _animationDuration) / 1000.0f);
	}

	return height*_screenPercentage;
}

//
////void default(const std::vector<std::string> & args) {
////    console->print(args[0]);
////    console->print(" is not a recognized command.\n");
////}
////
////void initialize() {
////
////    console->setDefaultCommand(default);
////}
// 
//void GUIConsole::passIntro()
//{
//    if(m_commandLine.length() > 0) parseCommandLine();
//}
// 
//
//bool GUIConsole::parseCommandLine()
//{
//    std::ostringstream out; // more info here
//    std::string::size_type index = 0;
//    std::vector<std::string> arguments;
//    std::list<console_item_t>::const_iterator iter;
//
//    // add to text buffer
//    //if(command_echo_enabled)
//    //{
//    //    print(m_commandLine);
//    //}
//
//    // add to commandline buffer
//    m_commandBuffer.push_back(m_commandLine);
//    //if(m_commandBuffer.size() > max_commands) m_commandBuffer.erase(m_commandBuffer.begin());
//
//    // tokenize
//    while(index != std::string::npos)
//    {
//        // push word
//        std::string::size_type next_space = m_commandLine.find(' ');
//        arguments.push_back(m_commandLine.substr(index, next_space));
//
//        // increment index
//        if(next_space != std::string::npos) index = next_space + 1;
//        else break;
//    }
//
//    // execute (look for the command or variable)
//    for(iter = m_itemsList.begin(); iter != m_ itemsList.end(); ++iter)
//    {
//        if(iter->name == arguments[0])
//        {
//            switch(iter->type)
//            {
//     
//            case CTYPE_UINT:
//                if(arguments.size() > 2)return false;
//                else if(arguments.size() == 1)
//                {
//                    out.str(""); // clear stringstream
//                    out << (*iter).name << " = " << *((unsigned int *)(*iter).variable_pointer);
//                    print(out.str());
//                    return true;
//                }
//                else if(arguments.size() == 2)
//                {
//                    *((unsigned int *)(*iter).variable_pointer) = (unsigned int) atoi(arguments[1].c_str());
//                    return true;
//                }
//                break;
//           
//            case CTYPE_FUNCTION:
//                (*iter).function(arguments);
//                return true;
//                break;
//           
//            default:
//                m_defaultCommand(arguments);
//                return false;
//                break;
//            }
//        }
//    }
//}

void GUIConsole::onKeyDown(const OIS::KeyEvent& key) {
}

void GUIConsole::onKeyUp(const OIS::KeyEvent& key) {
		switch( key.key ){
		case OIS::KC_GRAVE: ///tilde
			toggleConsole();
			break;
		default:
			break;
	}
}

void GUIConsole::onMouseMove(const OIS::MouseEvent& key) {
}

void GUIConsole::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button) {
}

void GUIConsole::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button) {
}