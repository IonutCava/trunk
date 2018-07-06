#ifndef _TEXT_3D_H_
#define _TEXT_3D_H_
#include "Geometry/Object3D.h"

class Text3D : public Object3D
{
public:
	Text3D(const std::string& text) : _text(text),_texture(NULL),_shader(NULL),_font(((void *)0x0000)/*GLUT_STROKE_ROMAN*/)
						 {_geometryType = TEXT_3D;}
	
	bool load(const std::string &name) {_text = name; return true;}
	bool unload() {_text.clear(); return true;}

	inline std::string&  getText()    {return _text;}
	inline void*		 getFont()    {return _font;}
	inline F32&			 getWidth()   {return _width;}
	inline Texture2D*	 getTexture() {return _texture;}
	inline Shader*		 getShader()  {return _shader;}

	virtual void computeBoundingBox()
	{
		vec3 min(-_width*2,0,-_width*0.5f);
		vec3 max(_width*1.5f*_text.length()*10,_width*_text.length()*1.5f,_width*0.5f);
		_bb.set(min,max);
		_bb.isComputed() = true;
		_originalBB = _bb;
	}

private:
	std::string _text;
	void* _font;
	F32   _width;
	Texture2D* _texture;
	Shader*   _shader;
};


#endif