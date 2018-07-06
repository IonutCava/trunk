#include "Hardware/Video/OpenGL/Headers/glResources.h"

#include "core.h"
#include "Headers/glTexture.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glTexture::glTexture(GLuint type, bool flipped) : Texture(flipped),
												  _type(type),
												  _internalFormat(GL_RGBA8),
												  _format(GL_RGBA)
{
	GLboolean textureCompression = glewIsSupported("GL_EXT_texture_compression_s3tc");
	_reservedStorage = false;
	_canReserveStorage = glewIsSupported("GL_ARB_texture_storage");
	if(textureCompression == GL_FALSE){
		ERROR_FN(Locale::get("ERROR_NO_S3TC_SUPPORT"));
		assert(textureCompression == GL_TRUE);
	}
}

glTexture::~glTexture()
{
}

void glTexture::threadedLoad(const std::string& name){
	Destroy();

	U32 tempHandle = 0;
	GLCheck(glGenTextures(1, &tempHandle));
	assert(tempHandle != 0);

	GLCheck(glBindTexture(_type,  tempHandle));

	GLCheck(glPixelStorei(GL_UNPACK_ALIGNMENT,1));
	GLCheck(glPixelStorei(GL_PACK_ALIGNMENT,1));

	if(_type == GL_TEXTURE_2D){
		/*GLCheck(glTexParameteri(_type, GL_TEXTURE_WRAP_S, glWrapTable[TEXTURE_REPEAT]));
		GLCheck(glTexParameteri(_type, GL_TEXTURE_WRAP_T, glWrapTable[TEXTURE_REPEAT]));*/
		if(!LoadFile(_type,name))	return;
	}else if(_type == GL_TEXTURE_CUBE_MAP){
		/*GLCheck(glTexParameteri(_type, GL_TEXTURE_WRAP_S, glWrapTable[TEXTURE_CLAMP_TO_EDGE]));
		GLCheck(glTexParameteri(_type, GL_TEXTURE_WRAP_T, glWrapTable[TEXTURE_CLAMP_TO_EDGE]));
		GLCheck(glTexParameteri(_type, GL_TEXTURE_WRAP_R, glWrapTable[TEXTURE_CLAMP_TO_EDGE]));*/

		GLbyte i=0;
		std::stringstream ss( name );
		std::string it;
		while(std::getline(ss, it, ' ')) {
			if(!LoadFile(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, it))	return;
			i++;
		}
        if(i != 6){
            ERROR_FN(Locale::get("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT"), name.c_str());
            return;
        }
	}

	if(_samplerDescriptor.generateMipMaps()){
		if(glGenerateMipmap != NULL)
			GLCheck(glGenerateMipmap(_type));
		else
			ERROR_FN(Locale::get("ERROR_NO_MIP_MAPS"));
	}

	Unbind();
	Texture::generateHWResource(name);
	Resource::threadedLoad(name);
	//Our "atomic swap"
	_handle = tempHandle;
}

void glTexture::setMipMapRange(U32 base, U32 max){
	if(!_samplerDescriptor.generateMipMaps()) return;

	Bind();
    GLCheck(glTexParameterf(_type, GL_TEXTURE_BASE_LEVEL, base));
    GLCheck(glTexParameterf(_type, GL_TEXTURE_MAX_LEVEL,  max));
	Unbind();
}

void glTexture::createSampler() {
	_sampler.Create(_samplerDescriptor);
}

bool glTexture::generateHWResource(const std::string& name) {
	//create the sampler first, even if it is the default one
	createSampler();
	GFX_DEVICE.loadInContext(_threadedLoading ? GFX_LOADING_CONTEXT : GFX_RENDERING_CONTEXT,
							 DELEGATE_BIND(&glTexture::threadedLoad, this, name));

	return true;
}

void glTexture::loadData(GLuint target, const GLubyte* const ptr, const vec2<U16>& dimensions, GLubyte bpp, GFXImageFormat format) {
	//If the current texture is a 2D one, than converting it to n^2 by n^2 dimensions will result in faster
	//rendering for the cost of a slightly higher loading overhead
	//The conversion code is based on the glmimg code from the glm library;
	size_t imageSize = (size_t)(dimensions.width) * (size_t)(dimensions.height) * (size_t)(bpp);
	GLubyte* img = New GLubyte[imageSize];
	memcpy(img, ptr, imageSize);

	_format = glImageFormatTable[format];
	_internalFormat = _format;
	switch(_format){
		case GL_RGB:  _internalFormat = GL_RGB8;  break;
		case GL_RGBA: _internalFormat = GL_RGBA8; break;
	}

	U32 width = dimensions.width;
	U32 height = dimensions.height;

    if (target == GL_TEXTURE_2D) {
		GLushort xSize2 = width, ySize2 = height;
		GLdouble xPow2 = log((GLdouble)xSize2) / log(2.0);
		GLdouble yPow2 = log((GLdouble)ySize2) / log(2.0);

		GLushort ixPow2 = (GLushort)xPow2;
		GLushort iyPow2 = (GLushort)yPow2;

		if (xPow2 != (GLdouble)ixPow2)   ixPow2++;
		if (yPow2 != (GLdouble)iyPow2)   iyPow2++;

		xSize2 = 1 << ixPow2;
		ySize2 = 1 << iyPow2;

		if((width != xSize2) || (height != ySize2)) {
			GLubyte* rdata = New GLubyte[xSize2*ySize2*bpp];
			GLCheck(gluScaleImage(_format, width, height, GL_UNSIGNED_BYTE, img, xSize2, ySize2, GL_UNSIGNED_BYTE, rdata));
			delete[] img;
			img = rdata;
			width = xSize2; height = ySize2;
		}
	}

	if(_canReserveStorage && _type != GL_TEXTURE_CUBE_MAP){
		if(!_reservedStorage) reserveStorage(width, height);
		GLCheck(glTexSubImage2D(target,0,0,0,width,height,_format,GL_UNSIGNED_BYTE,img));
	}else{
		GLCheck(glTexImage2D(target, 0, _internalFormat, width, height, 0, _format, GL_UNSIGNED_BYTE, img));
	}
}

void glTexture::reserveStorage(GLint w, GLint h){
	if(_canReserveStorage){
		GLint levels = 1;
		while ((w|h) >> levels) levels += 1;
		GLCheck(glTexStorage2D(_type,levels,_internalFormat,w,h));
	}
	_reservedStorage = true;
}

void glTexture::Destroy(){
	if(_handle > 0){
		U32 textureId = _handle;
		GLCheck(glDeleteTextures(1, &textureId));
		_handle = 0;
		_sampler.Destroy();
	}
}

void glTexture::Bind() const {
	GLCheck(glBindTexture(_type,  _handle));
	_sampler.Bind();
	Texture::Bind();
}

void glTexture::Bind(GLushort unit)  {
	while(!isHWInitComplete()){}
	if(checkBinding(unit, _handle)){ //prevent double bind
        GL_API::setActiveTextureUnit(unit);
		GLCheck(glBindTexture(_type, _handle));
		_sampler.Bind(unit);
	}
	Texture::Bind(unit);
}

void glTexture::Unbind() const {
	GLCheck(glBindTexture(_type, 0));
	_sampler.Unbind();
	Texture::Unbind();
}

void glTexture::Unbind(GLushort unit) {
    GL_API::setActiveTextureUnit(unit);
	GLCheck(glBindTexture(_type, 0));
	_sampler.Unbind(unit);
	Texture::Unbind(unit);
}