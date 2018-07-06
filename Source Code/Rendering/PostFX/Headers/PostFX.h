/*�Copyright 2009-2013 DIVIDE-Studio�*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _POST_EFFECTS_H
#define _POST_EFFECTS_H

#include "core.h"

#define FBO_BLOOM_SAMPLES 1

class GFXDevice;
class Texture;
typedef Texture Texture2D;
class Quad3D;
class Camera;
class ShaderProgram;
class FrameBufferObject;
DEFINE_SINGLETON( PostFX )

public:
	FrameBufferObject* _screenFBO;
	FrameBufferObject* _depthFBO;

	/// Depth of Field
	FrameBufferObject* _depthOfFieldFBO;

	/// Anaglyph
	FrameBufferObject* _anaglyphFBO[2];

	/// Bloom
	FrameBufferObject* _bloomFBO;

	///SSAO
	FrameBufferObject* _SSAO_FBO;

	/// Screen Border
	Texture2D*	_screenBorder;

	/// Noise
	Texture2D*	_noise;

	F32 _randomNoiseCoefficient, _randomFlashCoefficient;
	F32 _timer, _tickInterval;
	F32 _eyeOffset;

	Quad3D*	_renderQuad;
	ShaderProgram* _anaglyphShader;
	ShaderProgram* _postProcessingShader;
	Texture2D* _underwaterTexture;
	GFXDevice& _gfx;
	///Update the current camera at every render loop
	Camera*   _currentCamera;

private:
	void displaySceneWithoutAnaglyph(bool deferred);
	void displaySceneWithAnaglyph(bool deferred);
	void createOperators();
	~PostFX();
	PostFX();

	bool _enablePostProcessing;
	bool _enableAnaglyph;
	bool _enableBloom;
	bool _enableDOF;
	bool _enableNoise;
	bool _enableSSAO;
	bool _enableFXAA;
    bool _enableHDR;
	bool _FXAAinit;
	bool _underwater;

public:
	void init(const vec2<U16>& resolution);
	void idle();
	void render(Camera* const camera);
	void reshapeFBO(I32 newwidth , I32 newheight);

END_SINGLETON

#endif