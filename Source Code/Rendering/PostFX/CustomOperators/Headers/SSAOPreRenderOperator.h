#ifndef _SSAO_PRE_RENDER_OPERATOR_H_
#define _SSAO_PRE_RENDER_OPERATOR_H_
#include "Rendering/PostFX/Headers/PreRenderOperator.h"
///This Operator processes the entire geometry via the SSAOShader, generating a intermediate FBO
///The intermidiate FBO contains each object's normals in the "rgb" components, and the linear depth in the "a" component
///The intermidiate FBO is then passed as a 2D sampler in the second stage shader, processed and produces a full screen texure as a result
///The result FBO contains AO ambient values that should be added to the final fragment's ambient lighting value
class ShaderProgram;
class Quad3D;
class Texture;
typedef Texture Texture2D;
class FrameBufferObject;
class SSAOPreRenderOperator : public PreRenderOperator {
public:
	SSAOPreRenderOperator(ShaderProgram* const SSAOShader, Quad3D* const target, FrameBufferObject* result);
	~SSAOPreRenderOperator();

	void operation();
	void reshape(I32 width, I32 height);

private:
	ShaderProgram* _stage1Shader;
	ShaderProgram* _stage2Shader;
	FrameBufferObject* _outputFBO;
	FrameBufferObject* _normalsFBO;
	Texture2D*		   _colorNoise;

};

#endif