// compile with e.g.
// gcc  -o basicExample mainBasic.cpp -I"../../" ../../imgui.cpp ../../imgui_draw.cpp -D"IMGUI_INCLUDE_IMGUI_USER_H" -D"IMGUI_INCLUDE_IMGUI_USER_INL" -I"/usr/include/GLFW" -D"IMGUI_USE_GLFW_BINDING" -L"/usr/lib/x86_64-linux-gnu" -lglfw -lX11 -lm -lGL -lstdc++ -s

#include <imgui.h>	

void InitGL() {}
void ResizeGL(int w,int h) {}
void DestroyGL() {}
void DrawGL()	// Mandatory
{
   		ImImpl_ClearColorBuffer(ImVec4(0.6f, 0.6f, 0.6f, 1.0f));    // Warning: it does not clear the depth buffer

        static bool open = true;
        ImGui::Begin("Debug", &open, ImVec2(300,300)); 
        ImGui::Text("Hello, world!");
		ImGui::End();

		// However I got access to all addons from here now	
}


#ifndef IMGUI_USE_AUTO_BINDING_WINDOWS  // IMGUI_USE_AUTO_ definitions get defined automatically (e.g. do NOT touch them!)
int main(int argc, char** argv)	{
	ImImpl_Main(NULL,argc,argv);
	return 0;
}
#else // IMGUI_USE_AUTO_BINDING_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int iCmdShow)	{
	 ImImpl_WinMain(NULL,hInstance,hPrevInstance,lpCmdLine,iCmdShow);
}
#endif //IMGUI_USE_AUTO_BINDING_WINDOWS



