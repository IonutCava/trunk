#ifndef EFFECTS_H
#define EFFECTS_H

#include "resource.h"

#define FBO_BLOOM_SAMPLES 1

class Texture;
typedef Texture Texture2D;
class Quad3D;
class Shader;
class FrameBufferObject;
DEFINE_SINGLETON( PostFX )

public:
	FrameBufferObject* _screenFBO;
	FrameBufferObject* _depthFBO;

	// Depth of Field
	FrameBufferObject* _depthOfFieldFBO;
	FrameBufferObject* _tempDepthOfFieldFBO;

	// Anaglyph
	FrameBufferObject* _anaglyphFBO[2];

	// Bloom
	FrameBufferObject* _bloomFBO;
	FrameBufferObject* _tempBloomFBO;

	// Screen Border
	Texture2D*	_screenBorder;

	// Noise
	Texture2D*	_noise;
	
	F32 _randomNoiseCoefficient, _randomFlashCoefficient;
	F32 _timer, _tickInterval;
	F32 _eyeOffset;

	Quad3D*	_renderQuad;
	Shader* _anaglyphShader;
	Shader* _postProcessingShader;
	Shader* _blurShader;
	Shader* _bloomShader;
	Texture2D* _underwaterTexture;

private:
	void displaySceneWithoutAnaglyph(void);
	void displaySceneWithAnaglyph(void);
	void generateBloomTexture();
	void generateDepthOfFieldTexture();
	~PostFX();
	PostFX();

	bool _enablePostProcessing;
	bool _enableAnaglyph;
	bool _enableBloom;
	bool _enableDOF;
	bool _enableNoise;

public:
	void init();
	void idle();
	void render();
	void reshapeFBO(int newwidth , int newheight);

END_SINGLETON

#endif