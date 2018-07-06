#ifndef _BLOOM_PRE_RENDER_OPERATOR_H_
#define _BLOOM_PRE_RENDER_OPERATOR_H_
#include "Rendering/PostFX/Headers/PreRenderOperator.h"

class Shader;
class Quad3D;
class Texture;
typedef Texture Texture2D;
class FrameBufferObject;
class BloomPreRenderOperator : public PreRenderOperator {
public:
	BloomPreRenderOperator(Shader* const bloomShader, Quad3D* const target, FrameBufferObject* result);
	~BloomPreRenderOperator();

	void operation();
	void reshape(I32 width, I32 height);

private:
	Shader* _blur;
	Shader* _bright;
	FrameBufferObject* _outputFBO;
	FrameBufferObject* _tempBloomFBO;

};

#endif