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
                                                        _shaderProgramIDTemp(0)
{
    _vertexShader = nullptr;
    _fragmentShader = nullptr;
    _geometryShader = nullptr;
    _tessellationControlShader = nullptr;
    _tessellationEvaluationShader = nullptr;
    _computeShader = nullptr;
    _shaderProgramId = Divide::GLUtil::_invalidObjectID;

    _shaderStageTable[VERTEX_SHADER] = GL_VERTEX_SHADER;
    _shaderStageTable[FRAGMENT_SHADER] = GL_FRAGMENT_SHADER;
    _shaderStageTable[GEOMETRY_SHADER] = GL_GEOMETRY_SHADER;
    _shaderStageTable[TESSELATION_CTRL_SHADER] = GL_TESS_CONTROL_SHADER;
    _shaderStageTable[TESSELATION_EVAL_SHADER] = GL_TESS_EVALUATION_SHADER;
    _shaderStageTable[COMPUTE_SHADER] = GL_COMPUTE_SHADER;
}

glShaderProgram::~glShaderProgram()
{
    if (_shaderProgramId != 0 && _shaderProgramId != Divide::GLUtil::_invalidObjectID){
        FOR_EACH(ShaderIdMap::value_type& it, _shaderIdMap){
            it.second->removeParentProgram(this);
            glDetachShader(_shaderProgramId, it.second->getShaderId());
        }
        glDeleteProgram(_shaderProgramId);
    }
}

void glShaderProgram::validateInternal()  {
    glValidateProgram(_shaderProgramId);

    GLint status = 0;
    glGetProgramiv(_shaderProgramId, GL_VALIDATE_STATUS, &status);

    if (status == GL_FALSE){
        ERROR_FN(Locale::get("GLSL_VALIDATING_PROGRAM"), getName().c_str(), getLog().c_str());
    }
    else{
        D_PRINT_FN(Locale::get("GLSL_VALIDATING_PROGRAM"), getName().c_str(), getLog().c_str());
    }

    _validationQueued = false;
}

U8 glShaderProgram::update(const U64 deltaTime){
    //Validate the program after we used it once
    if(_validationQueued && isHWInitComplete() && _wasBound/*after it was used at least once*/){
        validateInternal();
#ifdef NDEBUG
        if (_shaderProgramId != 0 && _shaderProgramId != Divide::GLUtil::_invalidObjectID && !_loadedFromBinary){
            GLint   binaryLength;
   
            glGetProgramiv(_shaderProgramId, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
            void* binary = (void*)malloc(binaryLength);
            assert(binary != NULL);
            glGetProgramBinary(_shaderProgramId, binaryLength, nullptr, &_binaryFormat, binary);
 
            std::string outFileName("shaderCache/Binary/"+getName()+".bin");
            FILE* outFile = fopen(outFileName.c_str(), "wb");
            fwrite(binary, binaryLength, 1, outFile);
            fclose(outFile);
            free(binary);
        }
#endif
    }
      
    return ShaderProgram::update(deltaTime);
}

std::string glShaderProgram::getLog() const {
    std::string validationBuffer("[OK]");
    GLint length = 0;

    glGetProgramiv(_shaderProgramIDTemp, GL_INFO_LOG_LENGTH, &length);

    if(length > 1){
        validationBuffer = "\n -- ";
        vectorImpl<char> shaderProgramLog(length);
        glGetProgramInfoLog(_shaderProgramIDTemp, length, nullptr, &shaderProgramLog[0]);
        validationBuffer.append(&shaderProgramLog[0]);
        if(validationBuffer.size() > 4096 * 16){
            validationBuffer.resize(4096 * 16 - strlen(Locale::get("GLSL_LINK_PROGRAM_LOG")) - 10);
            validationBuffer.append(" ... ");
        }
    }

    return validationBuffer;
}

void glShaderProgram::detachShader(Shader* const shader){
    assert(_threadedLoadComplete);
    glDetachShader(_shaderProgramId ,shader->getShaderId());
}

void glShaderProgram::attachShader(Shader* const shader, const bool refresh){
    if (!shader)
        return;

    GLuint shaderId = shader->getShaderId();

    if(refresh){
        ShaderIdMap::iterator it = _shaderIdMap.find(shaderId);
        if(it != _shaderIdMap.end()){
            _shaderIdMap[shaderId] = shader;
            glDetachShader(_shaderProgramIDTemp, shaderId);
        }else{
            ERROR_FN(Locale::get("ERROR_SHADER_RECOMPILE_NOT_FOUND_ATOM"),shader->getName().c_str());
        }
    }else{
        _shaderIdMap.insert(std::make_pair(shaderId, shader));
    }

    glAttachShader(_shaderProgramIDTemp, shaderId);
    shader->addParentProgram(this);

    if (!shader->compile())
        ERROR_FN(Locale::get("ERROR_GLSL_SHADER_COMPILE"), shader->getShaderId());

    _linked = false;
}

void glShaderProgram::threadedLoad(const std::string& name){
    if (!_loadedFromBinary){
        //Create a new program
        if (_shaderProgramId == Divide::GLUtil::_invalidObjectID)
            _shaderProgramIDTemp = glCreateProgram();

        // Attach, compile and validate shaders into this program and update state
        attachShader(_vertexShader, _refreshVert);
        attachShader(_fragmentShader, _refreshFrag);
        attachShader(_geometryShader, _refreshGeom);
        attachShader(_tessellationControlShader, _refreshTess);
        attachShader(_tessellationEvaluationShader, _refreshTess);
        attachShader(_computeShader, _refreshComp);
        _refreshVert = _refreshFrag = _refreshGeom = _refreshTess = _refreshComp = false;
        link();
    }

    validate();
    _shaderProgramId = _shaderProgramIDTemp;

    ShaderProgram::generateHWResource(name);
}

void glShaderProgram::link(){
    GLint linkStatus = 0;
#ifdef NDEBUG
    if (Config::USE_SHADER_BINARY){
        glProgramParameteri(_shaderProgramIDTemp, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
    }
#endif
    D_PRINT_FN(Locale::get("GLSL_LINK_PROGRAM"), getName().c_str(), _shaderProgramIDTemp);

    if (_outputCount > 0){
        const char *vars[] = { "outVertexData", "outData1", "outData2", "outData3" };
        glTransformFeedbackVaryings(_shaderProgramIDTemp, _outputCount, vars, GL_SEPARATE_ATTRIBS);
    }

    glLinkProgram(_shaderProgramIDTemp);
    glGetProgramiv(_shaderProgramIDTemp, GL_LINK_STATUS, &linkStatus);

    if(linkStatus == GL_FALSE){
        ERROR_FN(Locale::get("GLSL_LINK_PROGRAM_LOG"), getName().c_str(), getLog().c_str());
    }else{
        D_PRINT_FN(Locale::get("GLSL_LINK_PROGRAM_LOG"), getName().c_str(), getLog().c_str());
    }
    _linked = true;
}

/// Creation of a new shader program.
/// Pass in a shader token and use glsw to load the corresponding effects
bool glShaderProgram::generateHWResource(const std::string& name){
    _name = name;
    
    //nullptr shader means use shaderProgram(0)
    if(name.compare("NULL_SHADER") == 0){
        _validationQueued = false;
        _shaderProgramId = 0;
        _threadedLoadComplete = HardwareResource::generateHWResource(name);
        return true;
    }

    //Set the compilation state
    _linked = false;
    //check for refresh flags
    bool refresh = _refreshVert || _refreshFrag || _refreshGeom || _refreshTess || _refreshComp;

#ifdef NDEBUG
    //Load the program from the binary file, if available, to avoid linking
    if (Config::USE_SHADER_BINARY && !refresh){
        FILE* inFile = fopen(std::string("shaderCache/Binary/" + _name + ".bin").c_str(), "rb");
        if (inFile){
            GLint   binaryLength;
            GLint   success;
            _shaderProgramIDTemp = glCreateProgram();
            fseek(inFile, 0, SEEK_END);
            binaryLength = (GLint)ftell(inFile);
            void* binary = (void*)malloc(binaryLength);
            fseek(inFile, 0, SEEK_SET);
            fread(binary, binaryLength, 1, inFile);
            fclose(inFile);
            glProgramBinary(_shaderProgramIDTemp, _binaryFormat, binary, binaryLength);
            free(binary);
            glGetProgramiv(_shaderProgramIDTemp, GL_LINK_STATUS, &success);

            if (success) {
                _loadedFromBinary = _linked = true;
                _threadedLoading = false;
            }
        }
    }
#endif
    if (!_linked){
        //Use the specified shader path
        glswSetPath(std::string(getResourceLocation() + "GLSL/").c_str(), ".glsl");
        //Mirror initial shader defines to match line count
        GLint lineCountOffset = 11;
        GLint lineCountOffsetFrag = 9;
        GLint lineCountOffsetVert = 8;
        GLint lineCountOffsetGeom = 0;
        if (GFX_DEVICE.getGPUVendor() == GPU_VENDOR_NVIDIA){ //nVidia specific
            lineCountOffset += 6;
        }

        std::string shaderSourceHeader;
        //get all of the preprocessor defines
        for (U8 i = 0; i < _definesList.size(); i++){
            if (_definesList[i].compare("DEFINE_PLACEHOLDER") == 0) continue;

            shaderSourceHeader.append("#define ");
            shaderSourceHeader.append(_definesList[i]);
            shaderSourceHeader.append("\n");
            lineCountOffset++;
        }

        std::string shaderSourceVertUniforms;
        //get all of the preprocessor defines
        for (U8 i = 0; i < _vertUniforms.size(); i++){
            shaderSourceVertUniforms.append("uniform ");
            shaderSourceVertUniforms.append(_vertUniforms[i]);
            shaderSourceVertUniforms.append(";\n");
            lineCountOffsetVert++;
        }
        std::string shaderSourceFragUniforms;
        //get all of the preprocessor defines
        for (U8 i = 0; i < _fragUniforms.size(); i++){
            shaderSourceFragUniforms.append("uniform ");
            shaderSourceFragUniforms.append(_fragUniforms[i]);
            shaderSourceFragUniforms.append(";\n");
            lineCountOffsetFrag++;
        }

        //Split the shader name to get the effect file name and the effect properties
        std::string shaderName = name.substr(0, name.find_first_of(".,"));
        std::string shaderProperties, vertexProperties;

        size_t propPositionVertex = name.find_first_of(",");
        size_t propPosition = name.find_first_of(".");

        if (propPosition != std::string::npos){
            shaderProperties = "." + name.substr(propPosition + 1, propPositionVertex - propPosition - 1);
        }

        vertexProperties += shaderProperties;

        if (propPositionVertex != std::string::npos){
            vertexProperties += "." + name.substr(propPositionVertex + 1);
        }

        _vertexShader = nullptr;
        //Load the Vertex Shader
        std::string vertexShaderName = shaderName + ".Vertex" + vertexProperties;

        //Recycle shaders only if we do not request a refresh for them
        if (!_refreshVert) {
            //See if it already exists in a compiled state
            _vertexShader = ShaderManager::getInstance().findShader(vertexShaderName, refresh);
        }

        //If it doesn't
        if (!_vertexShader){
            //Use glsw to read the vertex part of the effect
            const char* vs = glswGetShader(vertexShaderName.c_str(), lineCountOffset + lineCountOffsetVert, _refreshVert);
            if (!vs) ERROR_FN("[GLSL]  %s", glswGetError());
            assert(vs != nullptr);

            //If reading was successful
            std::string vsString(vs);
            Util::replaceStringInPlace(vsString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
            Util::replaceStringInPlace(vsString, "//__CUSTOM_VERTEX_UNIFORMS__",shaderSourceVertUniforms);
            //Load our shader and save it in the manager in case a new Shader Program needs it
            _vertexShader = ShaderManager::getInstance().loadShader(vertexShaderName, vsString, VERTEX_SHADER, _refreshVert);
        }

        if (!_vertexShader) ERROR_FN(Locale::get("WARN_GLSL_SHADER_LOAD"), vertexShaderName.c_str());
        assert(_vertexShader != nullptr);
        

        _fragmentShader = nullptr;
        //Load the Fragment Shader
        std::string fragmentShaderName = shaderName + ".Fragment" + shaderProperties;
        //Recycle shaders only if we do not request a refresh for them
        if (!_refreshFrag) {
            _fragmentShader = ShaderManager::getInstance().findShader(fragmentShaderName, refresh);
        }

        if (!_fragmentShader){
            //Get our shader source
            const char* fs = glswGetShader(fragmentShaderName.c_str(), lineCountOffset + lineCountOffsetFrag, _refreshFrag);
            if (fs){
                std::string fsString(fs);
                //Insert our custom defines in the special define slot
                Util::replaceStringInPlace(fsString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
                Util::replaceStringInPlace(fsString, "//__CUSTOM_FRAGMENT_UNIFORMS__", shaderSourceFragUniforms);
                //Load and compile the shader
                _fragmentShader = ShaderManager::getInstance().loadShader(fragmentShaderName, fsString, FRAGMENT_SHADER, _refreshFrag);
            }else{
                D_PRINT_FN(Locale::get("WARN_GLSL_SHADER_LOAD"), fragmentShaderName.c_str())
            }
        }

        if (!_fragmentShader) PRINT_FN(Locale::get("WARN_GLSL_SHADER_LOAD"), fragmentShaderName.c_str());

        _geometryShader = nullptr;
        //Load the Geometry Shader
        std::string geometryShaderName = shaderName + ".Geometry" + shaderProperties;
        //Recycle shaders only if we do not request a refresh for them
        if (!_refreshGeom) {
            _geometryShader = ShaderManager::getInstance().findShader(geometryShaderName, refresh);
        }

        if (!_geometryShader){
            const char* gs = glswGetShader(geometryShaderName.c_str(), lineCountOffset + lineCountOffsetGeom, _refreshGeom);
            if (gs != nullptr){
                std::string gsString(gs);
                Util::replaceStringInPlace(gsString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
                _geometryShader = ShaderManager::getInstance().loadShader(geometryShaderName, gsString, GEOMETRY_SHADER, _refreshGeom);
            }else{
                D_PRINT_FN(Locale::get("WARN_GLSL_SHADER_LOAD"), geometryShaderName.c_str());
            }
        }

        _tessellationControlShader = nullptr;
        _tessellationEvaluationShader = nullptr;
        //Load the Tessellation Shader
        std::string tessellationShaderControlName = shaderName + ".TessellationC" + shaderProperties;
        std::string tessellationShaderEvalName = shaderName + ".TessellationE" + shaderProperties;
        //Recycle shaders only if we do not request a refresh for them
        if (!_refreshTess) {
            _tessellationControlShader = ShaderManager::getInstance().findShader(tessellationShaderControlName, refresh);
            _tessellationEvaluationShader = ShaderManager::getInstance().findShader(tessellationShaderEvalName, refresh);
        }

        if (!_tessellationControlShader){
            const char* ts = glswGetShader(tessellationShaderControlName.c_str(), lineCountOffset, _refreshTess);
            if (ts != nullptr){
                std::string tsString(ts);
                Util::replaceStringInPlace(tsString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
                _tessellationControlShader = ShaderManager::getInstance().loadShader(tessellationShaderControlName, tsString, TESSELATION_CTRL_SHADER, _refreshTess);
            }
        }
        if (_tessellationControlShader && !_tessellationEvaluationShader){
            const char* ts = glswGetShader(tessellationShaderEvalName.c_str(), lineCountOffset, _refreshTess);
            if (ts != nullptr){
                std::string tsString(ts);
                Util::replaceStringInPlace(tsString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
                _tessellationEvaluationShader = ShaderManager::getInstance().loadShader(tessellationShaderEvalName, tsString, TESSELATION_EVAL_SHADER, _refreshTess);
            }
        }
        if (!_tessellationControlShader || _tessellationEvaluationShader){
            D_PRINT_FN(Locale::get("WARN_GLSL_SHADER_LOAD"), std::string(tessellationShaderControlName + " / " + tessellationShaderEvalName).c_str());
        }
        
        _computeShader = nullptr;
        //Load the Geometry Shader
        std::string computeShaderName = shaderName + ".Compute" + shaderProperties;
        //Recycle shaders only if we do not request a refresh for them
        if (!_refreshComp) {
            _computeShader = ShaderManager::getInstance().findShader(computeShaderName, refresh);
        }

        if (!_computeShader){
            const char* cs = glswGetShader(computeShaderName.c_str(), lineCountOffset, _refreshComp);
            if (cs != nullptr){
                std::string csString(cs);
                Util::replaceStringInPlace(csString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
                _computeShader = ShaderManager::getInstance().loadShader(computeShaderName, csString, COMPUTE_SHADER, _refreshComp);
            }else{
                D_PRINT_FN(Locale::get("WARN_GLSL_SHADER_LOAD"), computeShaderName.c_str());
            }
        }
    }
    _threadedLoading = false;
    return GFX_DEVICE.loadInContext(_threadedLoading ? GFX_LOADING_CONTEXT : GFX_RENDERING_CONTEXT, DELEGATE_BIND(&glShaderProgram::threadedLoad, this, name));
}

///Cache uniform/attribute locations for shader programs
///When we call this function, we check our name<->address map to see if we queried the location before
///If we did not, ask the GPU to give us the variables address and save it for later use
GLint glShaderProgram::cachedLoc(const std::string& name, const bool uniform){
    //Not loaded or NULL_SHADER
    if (!isHWInitComplete() || _shaderProgramId == 0 || _shaderProgramId == Divide::GLUtil::_invalidObjectID) return -1;
    
    assert(_linked && _threadedLoadComplete);

    const ShaderVarMap::iterator& it = _shaderVars.find(name);

    if(it != _shaderVars.end())	return it->second;

    GLint location = uniform ? glGetUniformLocation(_shaderProgramId, name.c_str()) : glGetAttribLocation(_shaderProgramId, name.c_str());

    _shaderVars[name] = location;
    return location;
}

bool glShaderProgram::bind() {
    //If we did not create the hardware resource, do not try and bind it, as it is invalid
    if (!isHWInitComplete()){
        GL_API::setActiveProgram(nullptr);
        return false;
    }

    assert(_shaderProgramId != Divide::GLUtil::_invalidObjectID);

    GL_API::setActiveProgram(this);
    //send default uniforms to GPU for every shader
   return ShaderProgram::bind();
}

void glShaderProgram::unbind(bool resetActiveProgram) {
    if(!_bound) return;

    if(resetActiveProgram) GL_API::setActiveProgram(nullptr);

    ShaderProgram::unbind(resetActiveProgram);
}

void glShaderProgram::Attribute(I32 location, GLdouble value) const {
    if (location == -1) return;

    glVertexAttrib1d(location,value);
}

void glShaderProgram::Attribute(I32 location, GLfloat value) const {
    if (location == -1) return;

    glVertexAttrib1f(location,value);
}

void glShaderProgram::Attribute(I32 location, const vec2<GLfloat>& value) const {
    if (location == -1) return;

    glVertexAttrib2fv(location,value);
}

void glShaderProgram::Attribute(I32 location, const vec3<GLfloat>& value) const {
    if (location == -1) return;

    glVertexAttrib3fv(location,value);
}

void glShaderProgram::Attribute(I32 location, const vec4<GLfloat>& value) const {
    if (location == -1) return;

    glVertexAttrib4fv(location,value);
}

void glShaderProgram::SetSubroutines(ShaderType type, const vectorImpl<U32>& indices) const {
    assert(_bound);
    if (indices[0] != GL_INVALID_INDEX)
        glUniformSubroutinesuiv(_shaderStageTable[type], (GLsizei)indices.size(), &indices.front());
    
}

U32 glShaderProgram::GetSubroutineIndex(ShaderType type, const std::string& name) const {
    return glGetSubroutineIndex(_shaderProgramId, _shaderStageTable[type], name.c_str());
}

void glShaderProgram::Uniform(GLint location, GLuint value) const {
    if (location == -1) return;

    if (!_bound) glProgramUniform1ui(_shaderProgramId, location, value);
    else         glUniform1ui(location, value);
}

void glShaderProgram::Uniform(GLint location, GLint value) const {
    if (location == -1) return;

    if (!_bound) glProgramUniform1i(_shaderProgramId, location, value);
    else         glUniform1i(location, value);
}

void glShaderProgram::Uniform(GLint location, GLfloat value) const {
    if (location == -1) return;

    if (!_bound) glProgramUniform1f(_shaderProgramId, location, value);
    else         glUniform1f(location, value);
}

void glShaderProgram::Uniform(GLint location, const vec2<GLfloat>& value) const {
    if (location == -1) return;

    if (!_bound) glProgramUniform2fv(_shaderProgramId, location, 1, value);
    else         glUniform2fv(location, 1, value);
}

void glShaderProgram::Uniform(GLint location, const vec2<GLint>& value) const {
    if (location == -1) return;

    if (!_bound) glProgramUniform2iv(_shaderProgramId, location, 1, value);
    else         glUniform2iv(location, 1, value);
}

void glShaderProgram::Uniform(GLint location, const vec2<GLushort>& value) const {
    if (location == -1) return;

    if (!_bound) glProgramUniform2iv(_shaderProgramId, location, 1, vec2<I32>(value.x, value.y));
    else         glUniform2iv(location, 1, vec2<I32>(value.x, value.y));
}

void glShaderProgram::Uniform(GLint location, const vec3<GLfloat>& value) const {
    if (location == -1) return;

    if (!_bound) glProgramUniform3fv(_shaderProgramId, location, 1, value);
    else         glUniform3fv(location, 1, value);
}

void glShaderProgram::Uniform(GLint location, const vec4<GLfloat>& value) const {
    if (location == -1) return;

    if (!_bound) glProgramUniform4fv(_shaderProgramId, location, 1, value);
    else         glUniform4fv(location, 1, value);
}

void glShaderProgram::Uniform(GLint location, const mat3<GLfloat>& value, bool rowMajor) const {
    if (location == -1) return;

    if (!_bound) glProgramUniformMatrix3fv(_shaderProgramId, location, 1, rowMajor, value.mat);
    else         glUniformMatrix3fv(location, 1, rowMajor, value);
}

void glShaderProgram::Uniform(GLint location, const mat4<GLfloat>& value, bool rowMajor) const {
    if (location == -1) return;

    if (!_bound) glProgramUniformMatrix4fv(_shaderProgramId, location, 1, rowMajor, value.mat);
    else         glUniformMatrix4fv(location, 1, rowMajor, value.mat);
}

void glShaderProgram::Uniform(GLint location, const vectorImpl<GLint >& values) const {
    if (values.empty()) return;

    if (!_bound) glProgramUniform1iv(_shaderProgramId, location, (GLsizei)values.size(), &values.front());
    else         glUniform1iv(location, (GLsizei)values.size(), &values.front());
}

void glShaderProgram::Uniform(GLint location, const vectorImpl<GLfloat >& values) const {
    if (values.empty() || location == -1) return;

    if (!_bound) glProgramUniform1fv(_shaderProgramId, location, (GLsizei)values.size(), &values.front());
    else         glUniform1fv(location, (GLsizei)values.size(), &values.front());
}

void glShaderProgram::Uniform(GLint location, const vectorImpl<vec2<GLfloat> >& values) const {
    if (values.empty() || location == -1) return;

    if (!_bound) glProgramUniform2fv(_shaderProgramId, location, (GLsizei)values.size(), values.front());
    else         glUniform2fv(location, (GLsizei)values.size(), values.front());
}

void glShaderProgram::Uniform(GLint location, const vectorImpl<vec3<GLfloat> >& values) const {
    if (values.empty() || location == -1) return;

    if (!_bound) glProgramUniform3fv(_shaderProgramId, location, (GLsizei)values.size(), values.front());
    else         glUniform3fv(location, (GLsizei)values.size(), values.front());
}

void glShaderProgram::Uniform(GLint location, const vectorImpl<vec4<GLfloat> >& values) const {
    if (values.empty() || location == -1) return;

    if (!_bound) glProgramUniform4fv(_shaderProgramId, location, (GLsizei)values.size(), values.front());
    else         glUniform4fv(location, (GLsizei)values.size(), values.front());
}

void glShaderProgram::Uniform(GLint location, const vectorImpl<mat3<F32> >& values, bool rowMajor) const {
    if (values.empty() || location == -1) return;

    if (!_bound) glProgramUniformMatrix3fv(_shaderProgramId, location, (GLsizei)values.size(), rowMajor, values.front());
    else         glUniformMatrix3fv(location, (GLsizei)values.size(), rowMajor, values.front());
}

void glShaderProgram::Uniform(GLint location, const vectorImpl<mat4<GLfloat> >& values, bool rowMajor) const {
    if (values.empty() || location == -1) return;

    if (!_bound) glProgramUniformMatrix4fv(_shaderProgramId, location, (GLsizei)values.size(), rowMajor, values.front());
    else         glUniformMatrix4fv(location, (GLsizei)values.size(), rowMajor, values.front());
}

void glShaderProgram::UniformTexture(GLint location, GLushort slot) const {
    if (location == -1) return;

    if(!_bound) glProgramUniform1i(_shaderProgramId, location, slot);
    else        glUniform1i(location, slot);
}