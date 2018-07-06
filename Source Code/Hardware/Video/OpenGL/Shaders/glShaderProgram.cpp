#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/OpenGL/glsw/Headers/glsw.h"

#include "core.h"
#include <glsl/glsl_optimizer.h>
#include "Headers/glShaderProgram.h"
#include "Headers/glUniformBufferObject.h"

#include "Hardware/Video/Shaders/Headers/Shader.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/ShaderManager.h"

glShaderProgram::glShaderProgram(const bool optimise) : ShaderProgram(optimise),
                                                        _validationQueued(false),
                                                        _loadedFromBinary(false),
                                                        _shaderProgramIdTemp(0)
{
    _shaderProgramId = Divide::GL::_invalidObjectID;
    //3 buffers: Matrices, Materials and Lights
    _UBOLocation.resize(UBO_PLACEHOLDER,-1);
    _uniformBufferObjects.resize(UBO_PLACEHOLDER, NULL);
    GL_API& glApi = GL_API::getInstance();
    _uniformBufferObjects[Lights_UBO]    = glApi.getUBO(Lights_UBO);
    _uniformBufferObjects[Matrices_UBO]  = glApi.getUBO(Matrices_UBO);
    _uniformBufferObjects[Materials_UBO] = glApi.getUBO(Materials_UBO);
}

glShaderProgram::~glShaderProgram()
{
    if(_shaderProgramId > 0 && _shaderProgramId != Divide::GL::_invalidObjectID){
        for_each(ShaderIdMap::value_type& it, _shaderIdMap){
            it.second->removeParentProgram(this);
            GLCheck(glDetachShader(_shaderProgramId, it.second->getShaderId()));
        }
        GLCheck(glDeleteProgram(_shaderProgramId));
    }

    _UBOLocation.clear();
    _uniformBufferObjects.clear();
}

U8 glShaderProgram::update(const U64 deltaTime){
    //Validate the program after we used it once
    if(_validationQueued && isHWInitComplete() && _wasBound/*after it was used at least once*/){
        validateInternal();
        flushLocCache();

        if(GL_API::_shaderBinarySupported && _shaderProgramId != 0 && !_loadedFromBinary){
            GLint   binaryLength;
            void*   binary;
            FILE*   outFile;
    
            GLCheck(glGetProgramiv(_shaderProgramId, GL_PROGRAM_BINARY_LENGTH, &binaryLength));
            binary = (void*)malloc(binaryLength);
            GLCheck(glGetProgramBinary(_shaderProgramId, binaryLength, NULL, &_binaryFormat, binary));
 
            std::string outFileName("shaderCache/Binary/"+getName()+".bin");
            outFile = fopen(outFileName.c_str(), "wb");
            fwrite(binary, binaryLength, 1, outFile);
            fclose(outFile);
            free(binary);
        }
    }
      
    return ShaderProgram::update(deltaTime);
}

std::string glShaderProgram::getLog() const {
    std::string validationBuffer("[OK]");
    GLint length = 0;

    GLCheck(glGetProgramiv(_threadedLoadComplete ? _shaderProgramId : _shaderProgramIdTemp, GL_INFO_LOG_LENGTH, &length));

    if(length > 1){
        validationBuffer = "\n -- ";
        std::vector<char> shaderProgramLog(length);
        GLCheck(glGetProgramInfoLog(_threadedLoadComplete ? _shaderProgramId : _shaderProgramIdTemp, length, NULL, &shaderProgramLog[0]));
        validationBuffer.append(&shaderProgramLog[0]);
        if(validationBuffer.size() > Console::CONSOLE_OUTPUT_BUFFER_SIZE){
            validationBuffer.resize(Console::CONSOLE_OUTPUT_BUFFER_SIZE - strlen(Locale::get("GLSL_LINK_PROGRAM_LOG")) - 10);
            validationBuffer.append(" ... ");
        }
    }

    return validationBuffer;
}

void glShaderProgram::validateInternal()  {
    GLCheck(glValidateProgram(_shaderProgramId));

    GLint status = 0;
    GLCheck(glGetProgramiv(_shaderProgramId, GL_VALIDATE_STATUS, &status));

    if (status == GL_FALSE){
        ERROR_FN(Locale::get("GLSL_VALIDATING_PROGRAM"), getName().c_str(), getLog().c_str());
    }else{
        D_PRINT_FN(Locale::get("GLSL_VALIDATING_PROGRAM"), getName().c_str(), getLog().c_str());
    }

    _validationQueued = false;
}

void glShaderProgram::detachShader(Shader* const shader){
    GLCheck(glDetachShader(_threadedLoadComplete ? _shaderProgramId : _shaderProgramIdTemp ,shader->getShaderId()));
}

void glShaderProgram::attachShader(Shader* const shader,const bool refresh){
    GLuint shaderId = shader->getShaderId();

    if(refresh){
        ShaderIdMap::iterator it = _shaderIdMap.find(shaderId);
        if(it != _shaderIdMap.end()){
            _shaderIdMap[shaderId] = shader;
            GLCheck(glDetachShader(_shaderProgramIdTemp, shaderId));
        }else{
            ERROR_FN(Locale::get("ERROR_SHADER_RECOMPILE_NOT_FOUND_ATOM"),shader->getName().c_str());
        }
    }else{
        _shaderIdMap.insert(std::make_pair(shaderId, shader));
    }

    GLCheck(glAttachShader(_shaderProgramIdTemp,shaderId));
    shader->addParentProgram(this);
   _compiled = false;
}

void glShaderProgram::threadedLoad(const std::string& name){
    //Link and validate shaders into this program and update state
    for_each(ShaderIdMap::value_type& it, _shaderIdMap){
        if(!it.second->compile()){
            ERROR_FN(Locale::get("ERROR_GLSL_SHADER_COMPILE"),it.second->getShaderId());
        }
    }
    //Load the program from the binary file, if available, to avoid linking
    if(GL_API::_shaderBinarySupported){
        GLint   binaryLength;
        GLint   success;
        void*   binary;
        FILE*   inFile;
        std::string inFileName("shaderCache/Binary/"+getName()+".bin");
        inFile = fopen(inFileName.c_str(), "rb");
        if(inFile){
            fseek(inFile, 0, SEEK_END);
            binaryLength = (GLint)ftell(inFile);
            binary = (void*)malloc(binaryLength);
            fseek(inFile, 0, SEEK_SET);
            fread(binary, binaryLength, 1, inFile);
            fclose(inFile);
            GLCheck(glProgramBinary(_shaderProgramIdTemp, _binaryFormat, binary, binaryLength));
            free(binary);
            GLCheck(glGetProgramiv(_shaderProgramIdTemp, GL_LINK_STATUS, &success));

            if(success){
                _loadedFromBinary = true;
                initUBO();
                validate();
                _shaderProgramId = _shaderProgramIdTemp;
                ShaderProgram::threadedLoad(name);
                return;
            }
        }
    }

    link();
    initUBO();
    validate();
    _shaderProgramId = _shaderProgramIdTemp;
    ShaderProgram::threadedLoad(name);
}

void glShaderProgram::initUBO(){
    GLCheck(_UBOLocation[Matrices_UBO]  =  glGetUniformBlockIndex(_shaderProgramIdTemp, "dvd_MatrixBlock"));
    GLCheck(_UBOLocation[Materials_UBO] =  glGetUniformBlockIndex(_shaderProgramIdTemp, "dvd_MaterialBlock"));
    GLCheck(_UBOLocation[Lights_UBO]    =  glGetUniformBlockIndex(_shaderProgramIdTemp, "dvd_LightBlock"));

    if(_UBOLocation[Matrices_UBO] != GL_INVALID_INDEX){
        _uniformBufferObjects[Matrices_UBO]->bindUniform(_shaderProgramIdTemp, _UBOLocation[Matrices_UBO]);
        _uniformBufferObjects[Matrices_UBO]->bindBufferBase();
    }
    if(_UBOLocation[Materials_UBO] != GL_INVALID_INDEX){
        _uniformBufferObjects[Materials_UBO]->bindUniform(_shaderProgramIdTemp, _UBOLocation[Materials_UBO]);
        _uniformBufferObjects[Materials_UBO]->bindBufferBase();
    }
    if(_UBOLocation[Lights_UBO] != GL_INVALID_INDEX){
        _uniformBufferObjects[Lights_UBO] ->bindUniform(_shaderProgramIdTemp, _UBOLocation[Lights_UBO]);
        _uniformBufferObjects[Lights_UBO] ->bindBufferBase();
    }
}

void glShaderProgram::link(){
    GLCheck(glBindAttribLocation(_shaderProgramIdTemp,Divide::GL::VERTEX_POSITION_LOCATION ,"inVertexData"));
    GLCheck(glBindAttribLocation(_shaderProgramIdTemp,Divide::GL::VERTEX_NORMAL_LOCATION   ,"inNormalData"));
    GLCheck(glBindAttribLocation(_shaderProgramIdTemp,Divide::GL::VERTEX_COLOR_LOCATION    ,"inColorData"));
    GLCheck(glBindAttribLocation(_shaderProgramIdTemp,Divide::GL::VERTEX_TEXCOORD_LOCATION ,"inTexCoordData"));
    GLCheck(glBindAttribLocation(_shaderProgramIdTemp,Divide::GL::VERTEX_TANGENT_LOCATION  ,"inTangentData"));
    GLCheck(glBindAttribLocation(_shaderProgramIdTemp,Divide::GL::VERTEX_BITANGENT_LOCATION,"inBiTangentData"));
    GLCheck(glBindAttribLocation(_shaderProgramIdTemp,Divide::GL::VERTEX_BONE_WEIGHT_LOCATION,"inBoneWeightData"));
    GLCheck(glBindAttribLocation(_shaderProgramIdTemp,Divide::GL::VERTEX_BONE_INDICE_LOCATION,"inBoneIndiceData"));

    GLint linkStatus = 0;
    if(GL_API::_shaderBinarySupported){
        GLCheck(glProgramParameteri(_shaderProgramIdTemp, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE));
    }
    D_PRINT_FN(Locale::get("GLSL_LINK_PROGRAM"),getName().c_str(), _shaderProgramIdTemp);
    GLCheck(glLinkProgram(_shaderProgramIdTemp));
    GLCheck(glGetProgramiv(_shaderProgramIdTemp, GL_LINK_STATUS, &linkStatus));

    if(linkStatus == GL_FALSE){
        ERROR_FN(Locale::get("GLSL_LINK_PROGRAM_LOG"), getName().c_str(), getLog().c_str());
    }else{
        D_PRINT_FN(Locale::get("GLSL_LINK_PROGRAM_LOG"), getName().c_str(), getLog().c_str());
    }
    _compiled = true;
}

/// Creation of a new shader program.
/// Pass in a shader token and use glsw to load the corresponding effects
bool glShaderProgram::generateHWResource(const std::string& name){
    _name = name;
    _threadedLoading = false;
    //NULL shader means use shaderProgram(0)
    if(name.compare("NULL_SHADER") == 0){
        _validationQueued = false;
        _shaderProgramId = 0;
        _threadedLoadComplete = HardwareResource::generateHWResource(name);
        return true;
    }

    //Set the compilation state
    _compiled = false;
    //check for refresh flags
    bool refresh = _refreshVert || _refreshFrag || _refreshGeom || _refreshTess;

    if(!refresh){
        //we need at least one shader for a shader program
        assert(_useVertex || _useFragment || _useGeometry || _useTessellation);
        //Create a new program
        GLCheck(_shaderProgramIdTemp = glCreateProgram());
     }

    //Use the specified shader path
    glswSetPath(std::string(getResourceLocation()+"GLSL/").c_str(), ".glsl");
    //Mirror initial shader defines to match line count
    GLint lineCountOffset = 11;
    GLint lineCountOffsetFrag = 0;
    GLint lineCountOffsetVert = 0;
    if(GFX_DEVICE.getGPUVendor() == GPU_VENDOR_NVIDIA){ //nVidia specific
        lineCountOffset += 5;
    }else if(GFX_DEVICE.getGPUVendor() == GPU_VENDOR_AMD){//AMD specific
        lineCountOffset += 1;
    }

    std::string shaderSourceHeader;
    //get all of the preprocessor defines
    for(U8 i = 0; i < _definesList.size(); i++){
        shaderSourceHeader.append("#define ");
        shaderSourceHeader.append(_definesList[i]);
        shaderSourceHeader.append("\n");
        lineCountOffset++;
    }

    std::string shaderSourceVertUniforms;
    //get all of the preprocessor defines
    for(U8 i = 0; i < _vertUniforms.size(); i++){
        shaderSourceVertUniforms.append("uniform ");
        shaderSourceVertUniforms.append(_vertUniforms[i]);
        shaderSourceVertUniforms.append(";\n");
        lineCountOffsetVert++;
    }
    std::string shaderSourceFragUniforms;
    //get all of the preprocessor defines
    for(U8 i = 0; i < _vertUniforms.size(); i++){
        shaderSourceFragUniforms.append("uniform ");
        shaderSourceFragUniforms.append(_fragUniforms[i]);
        shaderSourceFragUniforms.append(";\n");
        lineCountOffsetFrag++;
    }
    lineCountOffset += 1;//the last new line

    //Split the shader name to get the effect file name and the effect properties
    std::string shaderName = name.substr(0,name.find_first_of(".,"));
    std::string shaderProperties, vertexProperties;
    //Vertex Properties work in revers order. All the text after "," goes first.
    //The rest of the shader properties are added later
    size_t propPositionVertex = name.find_first_of(",");
    size_t propPosition = name.find_first_of(".");

    if(propPosition != std::string::npos){
        shaderProperties = "."+ name.substr(propPosition+1,propPositionVertex-propPosition-1);
    }

    if(propPositionVertex != std::string::npos){
        vertexProperties = "."+name.substr(propPositionVertex+1);
    }

    vertexProperties += shaderProperties;

    if(_useVertex){
        Shader* vertexShader = NULL;
        //Load the Vertex Shader
        std::string vertexShaderName = shaderName+".Vertex"+vertexProperties;

        //Recycle shaders only if we do not request a refresh for them
        if(!_refreshVert) {
            //See if it already exists in a compiled state
            vertexShader = ShaderManager::getInstance().findShader(vertexShaderName,refresh);
        }

        //If it doesn't
        if(!vertexShader){
            //Use glsw to read the vertex part of the effect
            const char* vs = glswGetShader(vertexShaderName.c_str(),lineCountOffset + lineCountOffsetVert,_refreshVert);
            if(!vs) ERROR_FN("[GLSL]  %s",glswGetError());
            assert(vs != NULL);

            //If reading was succesfull
            std::string vsString(vs);
            Util::replaceStringInPlace(vsString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
            //Util::replaceStringInPlace(vsString, "//__CUSTOM_VERTEX_UNIFORMS__",shaderSourceVertUniforms);
            //Load our shader and save it in the manager in case a new Shader Program needs it
            vertexShader = ShaderManager::getInstance().loadShader(vertexShaderName,vsString,VERTEX_SHADER,_refreshVert);
        }

        if(!vertexShader) ERROR_FN(Locale::get("ERROR_GLSL_SHADER_LOAD"),vertexShaderName.c_str());
        assert(vertexShader != NULL);

        //If the vertex shader loaded ok attach it to the program if needed
        if(!refresh || refresh && _refreshVert){
            attachShader(vertexShader, _refreshVert);
        }
    }

    if(_useFragment){
        Shader* fragmentShader = NULL;
        //Load the Fragment Shader
        std::string fragmentShaderName = shaderName+".Fragment"+shaderProperties;
        //Recycle shaders only if we do not request a refresh for them
        if(!_refreshFrag) {
            fragmentShader = ShaderManager::getInstance().findShader(fragmentShaderName,refresh);
        }

        if(!fragmentShader){
            //Get our shader source
            const char* fs = glswGetShader(fragmentShaderName.c_str(),lineCountOffset + lineCountOffsetFrag, _refreshFrag);
            if(!fs) ERROR_FN("[GLSL] %s",glswGetError());
            assert(fs != NULL);

            std::string fsString(fs);
            //Insert our custom defines in the special define slot
            Util::replaceStringInPlace(fsString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
            Util::replaceStringInPlace(fsString, "//__CUSTOM_FRAGMENT_UNIFORMS__",shaderSourceFragUniforms);
            //Load and compile the shader
            fragmentShader = ShaderManager::getInstance().loadShader(fragmentShaderName,fsString,FRAGMENT_SHADER,_refreshFrag);
        }

        if(!fragmentShader) ERROR_FN(Locale::get("ERROR_GLSL_SHADER_LOAD"),fragmentShaderName.c_str());
        assert(fragmentShader != NULL);
        //attach it to the program if needed
        if(!refresh || refresh && _refreshFrag){
            attachShader(fragmentShader, _refreshFrag);
        }
    }

    if(_useGeometry){
        Shader* geometryShader = NULL;
        //Load the Geometry Shader
        std::string geometryShaderName = shaderName+".Geometry"+shaderProperties;
        //Recycle shaders only if we do not request a refresh for them
        if(!_refreshGeom) {
            geometryShader = ShaderManager::getInstance().findShader(geometryShaderName,refresh);
        }

        if(!geometryShader){
            const char* gs = glswGetShader(geometryShaderName.c_str(),lineCountOffset,_refreshGeom);
            if(gs != NULL){
                std::string gsString(gs);
                Util::replaceStringInPlace(gsString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
                geometryShader = ShaderManager::getInstance().loadShader(geometryShaderName,gsString,GEOMETRY_SHADER,_refreshGeom);
            }else{
                //Use debug output for geometry and tessellation shaders as they are not vital for the application as of yet
                D_ERROR_FN("[GLSL] %s", glswGetError());
            }
        }
        if(!geometryShader){
            D_ERROR_FN(Locale::get("ERROR_GLSL_SHADER_LOAD"),geometryShaderName.c_str());
        }else{
            //attach it to the program if needed
            if(!refresh || refresh && _refreshGeom){
                attachShader(geometryShader, _refreshGeom);
            }
        }
    }

    if(_useTessellation){
        Shader* tessellationShader = NULL;
        //Load the Tessellation Shader
        std::string tessellationShaderName = shaderName+".Tessellation"+shaderProperties;
        //Recycle shaders only if we do not request a refresh for them
        if(!_refreshTess) {
            tessellationShader = ShaderManager::getInstance().findShader(tessellationShaderName,refresh);
        }

        if(!tessellationShader){
            const char* ts = glswGetShader(tessellationShaderName.c_str(),lineCountOffset,_refreshTess);
            if(ts != NULL){
                std::string tsString(ts);
                Util::replaceStringInPlace(tsString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
                tessellationShader = ShaderManager::getInstance().loadShader(tessellationShaderName,tsString,TESSELATION_SHADER,_refreshTess);
            }else{
                //Use debug output for geometry and tessellation shaders as they are not vital for the application as of yet
                D_ERROR_FN("[GLSL] %s", glswGetError());
            }
        }
        if(!tessellationShader){
            D_ERROR_FN(Locale::get("ERROR_GLSL_SHADER_LOAD"),tessellationShaderName.c_str());
        }else{
            //attach it to the program if needed
            if(!refresh || refresh && _refreshTess){
                attachShader(tessellationShader, _refreshTess);
            }
        }
    }

    _refreshTess = _refreshGeom = _refreshVert = _refreshFrag = false;

    GFX_DEVICE.loadInContext(_threadedLoading ? GFX_LOADING_CONTEXT : GFX_RENDERING_CONTEXT,
                             DELEGATE_BIND(&glShaderProgram::threadedLoad, this, name));
    return true;
}

///Cache uniform/attribute locations for shaderprograms
///When we call this function, we check our name<->address map to see if we queried the location before
///If we did not, ask the GPU to give us the variables address and save it for later use
GLint glShaderProgram::cachedLoc(const std::string& name, const bool uniform){
    //Not loaded or NULL_SHADER
    if(_shaderProgramId == 0) return -1;

    while(!isHWInitComplete()){}

    ShaderVarMap::iterator it = _shaderVars.find(name);

    if(it != _shaderVars.end())	return it->second;

    GLint location = -1;
    if(uniform)	GLCheck(location = glGetUniformLocation(_shaderProgramId, name.c_str()));
    else   	    GLCheck(location = glGetAttribLocation(_shaderProgramId, name.c_str()));

    _shaderVars.insert(std::make_pair(name,location));
    return location;
}

void glShaderProgram::bind() {
    //If we did not create the hardware resource, do not try and bind it, as it is invalid
    while(!isHWInitComplete()){}
    assert(_shaderProgramId != Divide::GL::_invalidObjectID);

    GL_API::setActiveProgram(this);

    //send default uniforms to GPU for every shader
    ShaderProgram::bind();
}

void glShaderProgram::unbind(bool resetActiveProgram) {
    if(!_bound) return;

    if(resetActiveProgram) GL_API::setActiveProgram(NULL);

    ShaderProgram::unbind(resetActiveProgram);
}

void glShaderProgram::Attribute(const std::string& ext, GLdouble value){
    GLint location = cachedLoc(ext,false);
    if(location == -1) return;
    GLCheck(glVertexAttrib1d(location,value));
}
void glShaderProgram::Attribute(const std::string& ext, GLfloat value){
    GLint location = cachedLoc(ext,false);
    if(location == -1) return;
    GLCheck(glVertexAttrib1f(location,value));
}
void glShaderProgram::Attribute(const std::string& ext, const vec2<GLfloat>& value){
    GLint location = cachedLoc(ext,false);
    if(location == -1) return;
    GLCheck(glVertexAttrib2fv(location,value));
}
void glShaderProgram::Attribute(const std::string& ext, const vec3<GLfloat>& value){
    GLint location = cachedLoc(ext,false);
    if(location == -1) return;
    GLCheck(glVertexAttrib3fv(location,value));
}

void glShaderProgram::Attribute(const std::string& ext, const vec4<GLfloat>& value){
    GLint location = cachedLoc(ext,false);
    if(location == -1) return;
    GLCheck(glVertexAttrib4fv(location,value));
}

void glShaderProgram::Uniform(const std::string& ext, GLuint value){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform1ui(_shaderProgramId, location, value));
    else        GLCheck(glUniform1ui(location, value));
}

void glShaderProgram::Uniform(const std::string& ext, GLint value){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform1i(_shaderProgramId, location, value));
    else        GLCheck(glUniform1i(location, value));
}

void glShaderProgram::Uniform(const std::string& ext, GLfloat value){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform1f(_shaderProgramId, location, value));
    else        GLCheck(glUniform1f(location, value));
}

void glShaderProgram::Uniform(const std::string& ext, const vec2<GLfloat>& value){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform2fv(_shaderProgramId, location, 1, value));
    else        GLCheck(glUniform2fv(location, 1, value));
}

void glShaderProgram::Uniform(const std::string& ext, const vec2<GLint>& value){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform2iv(_shaderProgramId, location, 1, value));
    else        GLCheck(glUniform2iv(location, 1, value));
}

void glShaderProgram::Uniform(const std::string& ext, const vec2<GLushort>& value){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform2iv(_shaderProgramId, location, 1, vec2<I32>(value.x, value.y)));
    else        GLCheck(glUniform2iv(location, 1, vec2<I32>(value.x, value.y)));
}

void glShaderProgram::Uniform(const std::string& ext, const vec3<GLfloat>& value){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform3fv(_shaderProgramId, location, 1, value));
    else        GLCheck(glUniform3fv(location, 1, value));
}

void glShaderProgram::Uniform(const std::string& ext, const vec4<GLfloat>& value){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform4fv(_shaderProgramId, location, 1, value));
    else        GLCheck(glUniform4fv(location, 1, value));
}

void glShaderProgram::Uniform(const std::string& ext, const mat3<GLfloat>& value, bool rowMajor){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniformMatrix3fv(_shaderProgramId, location, 1, rowMajor, value));
    else        GLCheck(glUniformMatrix3fv(location, 1,rowMajor, value));
}

void glShaderProgram::Uniform(const std::string& ext, const mat4<GLfloat>& value, bool rowMajor){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniformMatrix4fv(_shaderProgramId, location, 1, rowMajor, value));
    else        GLCheck(glUniformMatrix4fv(location, 1, rowMajor, value.mat));
}

void glShaderProgram::Uniform(const std::string& ext, const vectorImpl<GLint >& values){
    if(values.empty()) return;
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform1iv(_shaderProgramId, location, values.size(),&values.front()));
    else        GLCheck(glUniform1iv(location,values.size(),&values.front()));
}

void glShaderProgram::Uniform(const std::string& ext, const vectorImpl<GLfloat >& values){
    if(values.empty()) return;
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform1fv(_shaderProgramId, location, values.size(),&values.front()));
    else        GLCheck(glUniform1fv(location,values.size(),&values.front()));
}

void glShaderProgram::Uniform(const std::string& ext, const vectorImpl<vec2<GLfloat> >& values) {
    if(values.empty()) return;
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform2fv(_shaderProgramId, location, values.size(), values.front()));
    else        GLCheck(glUniform2fv(location, values.size(), values.front()));
}

void glShaderProgram::Uniform(const std::string& ext, const vectorImpl<vec3<GLfloat> >& values) {
    if(values.empty()) return;
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform3fv(_shaderProgramId, location, values.size(), values.front()));
    else        GLCheck(glUniform3fv(location, values.size(), values.front()));
}

void glShaderProgram::Uniform(const std::string& ext, const vectorImpl<vec4<GLfloat> >& values) {
    if(values.empty()) return;
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform4fv(_shaderProgramId, location, values.size(), values.front()));
    else        GLCheck(glUniform4fv(location, values.size(), values.front()));
}

void glShaderProgram::Uniform(const std::string& ext, const vectorImpl<mat4<GLfloat> >& values, bool rowMajor){
    if(values.empty()) return;
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniformMatrix4fv(_shaderProgramId, location, values.size(),rowMajor, values.front()));
    else        GLCheck(glUniformMatrix4fv(location, values.size(), rowMajor, values.front()));
}

void glShaderProgram::UniformTexture(const std::string& ext, GLushort slot){
    GLint location = cachedLoc(ext);
    if(location == -1) return;

    if(!_bound) GLCheck(glProgramUniform1i(_shaderProgramId, location, slot));
    else        GLCheck(glUniform1i(location, slot));
}