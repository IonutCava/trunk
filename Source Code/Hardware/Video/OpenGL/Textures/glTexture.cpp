#include "Hardware/Video/OpenGL/Headers/glResources.h"

#include "core.h"
#include "Headers/glTexture.h"
#include "Hardware/Video/Headers/GFXDevice.h"

glTexture::glTexture(GLuint type, bool flipped) : Texture(flipped),
                                                  _type(type),
                                                  _allocatedStorage(false)
{

}

glTexture::~glTexture()
{
}

void glTexture::threadedLoad(const std::string& name){
    Destroy();

    U32 tempHandle = 0;
    glGenTextures(1, &tempHandle);
    assert(tempHandle != 0);

    GL_API::bindTexture(0, tempHandle, _type);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glPixelStorei(GL_PACK_ALIGNMENT,1);

    if(_type == GL_TEXTURE_2D){
        if(!LoadFile(_type,name))	return;
    }else if (_type == GL_TEXTURE_CUBE_MAP || _type == GL_TEXTURE_2D_ARRAY){
        GLbyte i=0;
        std::stringstream ss( name );
        std::string it;
        while(std::getline(ss, it, ' ')) {
            if (!it.empty()){
                if(!LoadFile(i, it)) return;
                i++;
            }
        }
        if (i != 6 && _type == GL_TEXTURE_CUBE_MAP){
            ERROR_FN(Locale::get("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT"), name.c_str());
            return;
        }
    }

    if(_samplerDescriptor.generateMipMaps()){
        if(glGenerateMipmap != nullptr){
            glGenerateMipmap(_type);
        }else{
            ERROR_FN(Locale::get("ERROR_NO_MIP_MAPS"));
            assert(glGenerateMipmap);
        }
    }

    GL_API::unbindTexture(0, _type);
    Texture::generateHWResource(name);
    Resource::threadedLoad(name);
    //Our "atomic swap"
    _handle = tempHandle;
}

void glTexture::setMipMapRange(U32 base, U32 max){
    if(!_samplerDescriptor.generateMipMaps()) return;

    glTextureParameterfEXT(_handle, _type, GL_TEXTURE_BASE_LEVEL, base);
    glTextureParameterfEXT(_handle, _type, GL_TEXTURE_MAX_LEVEL, max);
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
    GLenum formatGL = glImageFormatTable[format];
    GLenum internalFormatGL = formatGL;
    GLuint width = dimensions.width;
    GLuint height = dimensions.height;

    switch (formatGL){
        case GL_RGB:  internalFormatGL = GL_RGB8;  break;
        case GL_RGBA: internalFormatGL = GL_RGBA8; break;
    }

    if (_type != GL_TEXTURE_CUBE_MAP) {
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
            gluScaleImage(formatGL, width, height, GL_UNSIGNED_BYTE, img, xSize2, ySize2, GL_UNSIGNED_BYTE, rdata);
            SAFE_DELETE_ARRAY(img);
            img = rdata;
            width = xSize2; height = ySize2;
        }
    }
    assert(width > 0 && height > 0);
    GLint numLevels = (GLint)floorf(log2f(fmaxf(width, height)));
    setMipMapRange(0, numLevels);

    if (_type == GL_TEXTURE_2D_ARRAY){
        if (!_allocatedStorage){
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, numLevels, internalFormatGL, width, height, _numLayers);
            _allocatedStorage = true;
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, target, width, height, 1, formatGL, GL_UNSIGNED_BYTE, img);
    }else{
        if (!_allocatedStorage){
            glTexStorage2D(_type, numLevels, internalFormatGL, width, height);
            _allocatedStorage = true;
        }
        GLenum texTarget = _type == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + target : target;
        glTexSubImage2D(texTarget, 0, 0, 0, width, height, formatGL, GL_UNSIGNED_BYTE, img);

    }
    SAFE_DELETE_ARRAY(img);
}

void glTexture::Destroy(){
    if(_handle > 0){
        U32 textureId = _handle;
        glDeleteTextures(1, &textureId);
        _handle = 0;
        _sampler.Destroy();
    }
}

void glTexture::Bind(GLushort unit)  {
    GL_API::bindTexture(unit, _handle, _type, _sampler.getObjectHandle());
}