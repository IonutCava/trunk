#include <imgui.h>	

// These are just the fonts we're going to load in main(...). They're not mandatory (see InitGL() too)
enum MyFontEnum {
FNT_NORMAL=0,
FNT_BOLD,
FNT_ITALIC,
FNT_BOLDITALIC
};





void InitGL() {
    ImGuiIO& io = ImGui::GetIO();

    // It's not mandatory to have all the font style... you can skip some, you can even skip this call if you want
    ImGuiCe::CodeEditor::SetFonts(ImGui::GetFont(FNT_NORMAL),ImGui::GetFont(FNT_BOLD),ImGui::GetFont(FNT_ITALIC),ImGui::GetFont(FNT_BOLDITALIC));

    //  Optional CTRL + MW to zoom
    io.FontAllowUserScaling = true;

//#   define PERFORM_TESTS
#   ifdef PERFORM_TESTS
    ImHashMapString hashMap;int value=-1;
    char name[]="Ciao";
    hashMap.put(name, 1);hashMap.put("Goodnight", 2);hashMap.put("Asta la vista", 3);hashMap.put("Miao", 3);
    //name[2]='\0';   // This line makes test 2 fail because strings are not copied!
    if (hashMap.get("Goodnight", value))    fprintf(stderr,"1) %d\n",value);
    else                                    fprintf(stderr,"1) FAILED\n");
    if (hashMap.get("Ciao", value))         fprintf(stderr,"2) %d\n",value);
    else                                    fprintf(stderr,"2) FAILED\n");
    if (hashMap.get("Miao", value))         fprintf(stderr,"3) %d\n",value);
    else                                    fprintf(stderr,"3) FAILED\n");
    hashMap.remove("Goodnight");
    if (hashMap.get("Goodnight", value))    fprintf(stderr,"4) FAILED\n");
    else                                    fprintf(stderr,"4) PASSED\n");
#   endif //PERFORM_TESTS

}
void ResizeGL(int w,int h) {}
void DestroyGL() {}
void DrawGL()	// Mandatory
{
    ImGuiIO& io = ImGui::GetIO();

    ImImpl_ClearColorBuffer(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));    // Warning: it does not clear depth buffer

    static bool open = true;
    ImGui::SetNextWindowSize(ImVec2(800,600));
    if (ImGui::Begin("imguicodeeditor (WIP: UNUSABLE)", &open)) {
    //io.FontDefault = io.Fonts->Fonts[FNT_ITALIC];
    //ImGui::Text("Hello, world!");   // This changes if we change the dafault ImGui Font
    //ImGui::TextColored(FNT_BOLDITALIC,KNOWNIMGUICOLOR_YELLOW,"%s","Hello,");ImGui::SameLine(0,0);ImGui::Text(FNT_NORMAL,"%s"," world!");   // This stays the same
	
    static ImGuiCe::CodeEditor ce;
    if (!ce.isInited()) {        
        ce.init();  // optional
        ce.show_left_pane = true;   //dbg
        // Load here something or use ce.setText() here...
        static const char* myCode="# include <sadd.h>\n\nusing namespace std;\n\n//This is a comment\nclass MyClass\n{\npublic:\nMyClass() {}\nvoid Init(int num)\n{  // for loop\nfor (int t=0;t<20;t++)\n	{\n     mNum=t; /* setting var */\n     const float myFloat = 1.25f;\n      break;\n	}\n}\n\nprivate:\nint mNum;\n};\n\nstatic const char* SomeStrings[] = {\"One\"/*Comment One*//*Comment*/,\"Two /*Fake Comment*/\",\"Three\\\"Four\"};\n\nwhile (i<25 && i>=0)   {\n\ti--;\nbreak;} /*comment*/{/*This should not fold*/}/*comment2*/for (int i=0;i<20;i++)    {\n\t\t\tcontinue;//OK\n} // end second folding\n\nfor (int j=0;j<200;j++)  {\ncontinue;}\n\n//region Custom Region Here\n{\n//something inside here\n}\n//endregion\n\n/*\nMultiline\nComment\nHere\n*/\n\n/*\nSome Unicode Characters here:\n€€€€\n*/\n\n";
        ce.setText(myCode,ImGuiCe::LANG_CPP);
    }
    ce.render();

    //static const std::string myText = ce.Lines.getText();
    }
    ImGui::End();

    //ImGui::ShowTestWindow();

}




#ifndef IMGUI_USE_AUTO_BINDING_WINDOWS  // IMGUI_USE_AUTO_ definitions get defined automatically (e.g. do NOT touch them!)
int main(int argc, char** argv)
#else // IMGUI_USE_AUTO_BINDING_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int iCmdShow)   // This branch has made my code less concise (I will consider stripping it)
#endif //IMGUI_USE_AUTO_BINDING_WINDOWS
{    
    const float fontSize = 13.f;                        // 13.f allows us to reuse the default ImGui font as NORMAL font if we want.
    const bool useDefaultImGuiFontAsNormalFont = false;
    ImFontConfig fcfg;fcfg.OversampleH = 2;             // By default is 3: looks better, but takes up 3 times the texture space
    static const ImWchar glyphRanges[] =                // Must be static
        {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x20AC, 0x20AC,	// €
            0x2122, 0x2122,	// ™
            0x2196, 0x2196, // ↖
            0x21D6, 0x21D6, // ⇖
            0x2B01, 0x2B01, // ⬁
            0x2B09, 0x2B09, // ⬉
            0x2921, 0x2922, // ⤡ ⤢
            0x263A, 0x263A, // ☺
            0x266A, 0x266A, // ♪
            0x2328, 0x2328, // ⌨            (it's a PC keyboard)
            0x23CF, 0x23CF, // ⏏            (eject)
            0x25B2, 0x25B5, // ▲ △ ▴ ▵      (up arrows)
            0x25BC, 0x25BF, // ▼ ▽ ▾ ▿      (down arrows)
            0x25B6, 0x25BB, // ▶ ▷ ▸ ▹ ► ▻  (right arrows)
            0x25C0, 0x25C5, // ◀ ◁ ◂ ◃ ◄ ◅  (left arrows)
            0x25A0, 0x25A3, // ■ □ ▢ ▣      (quads good fo checkboxes)
            0 // € ™ ↖ ⇖ ⬁ ⬉ ⤡ ⤢ ☺ ♪
        };

    ImVector<ImImpl_InitParams::FontData> fnts; // currently from file supports only .ttf (and .ttf.gz if IMGUI_USE_ZLIB is defined at the project level). From memory all the available ImImpl_InitParams::FontData::COMP_XXX.    
    if (!useDefaultImGuiFontAsNormalFont) fnts.push_back(ImImpl_InitParams::FontData("fonts/Mono/DejaVuSansMono-Stripped.ttf",fontSize,NULL));//glyphRanges,&fcfg));
    fnts.push_back(ImImpl_InitParams::FontData("fonts/Mono/DejaVuSansMono-Bold-Stripped.ttf",fontSize,glyphRanges,&fcfg));
    fnts.push_back(ImImpl_InitParams::FontData("fonts/Mono/DejaVuSansMono-Oblique-Stripped.ttf",fontSize,glyphRanges,&fcfg));
    fnts.push_back(ImImpl_InitParams::FontData("fonts/Mono/DejaVuSansMono-BoldOblique-Stripped.ttf",fontSize,glyphRanges,&fcfg));

/*  // Tip: we can load embed the fonts in code and load them from memory instead (Warning: embedding fonts may imply problems with the font license). An example:
    const unsigned char myInlineFont[] =
#   include "fonts/Mono/DejaVuSansMono-Bold.ttf.gz.inl"
    fnts.push_back(ImImpl_InitParams::FontData(myInlineFont,sizeof(myInlineFont),ImImpl_InitParams::FontData::COMP_GZ,13.f,glyphRanges,&fcfg));
*/
//  Tip: open source programs like fontforge can be used to strip .ttf files from unused characters to reduce their size considerably.

    ImImpl_InitParams gImGuiInitParams(1024,720,NULL,fnts,useDefaultImGuiFontAsNormalFont);    // w,h,title,fnts,addDefaultFontAsZeroFont
    gImGuiInitParams.gFpsClampInsideImGui = 30.0f;  // Optional Max allowed FPS (default -1 => unclamped). Useful for editors and to save GPU and CPU power.
    gImGuiInitParams.gFpsDynamicInsideImGui = false; // If true when inside ImGui, the FPS is not constant (at gFpsClampInsideImGui), but goes from a very low minimum value to gFpsClampInsideImGui dynamically. Useful for editors and to save GPU and CPU power.
    gImGuiInitParams.gFpsClampOutsideImGui = 10.f;  // Optional Max allowed FPS (default -1 => unclamped). Useful for setting a different FPS for your main rendering.


#ifndef IMGUI_USE_AUTO_BINDING_WINDOWS
    ImImpl_Main(&gImGuiInitParams,argc,argv);
#else // IMGUI_USE_AUTO_BINDING_WINDOWS
    ImImpl_WinMain(&gImGuiInitParams,hInstance,hPrevInstance,lpCmdLine,iCmdShow);
#endif //IMGUI_USE_AUTO_BINDING_WINDOWS

    return 0;
}

