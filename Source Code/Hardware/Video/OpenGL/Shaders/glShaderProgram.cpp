#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/OpenGL/glsw/Headers/glsw.h"
#include "Hardware/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"

#include "core.h"
#include <glsl/glsl_optimizer.h>
#include "Headers/glShaderProgram.h"

#include "Hardware/Video/Shaders/Headers/Shader.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/ShaderManager.h"

glShaderProgram::glShaderProgram(const bool optimise) : ShaderProgram(optimise),
                                                        _validationQueued(false),
                                                        _loadedFromBinary(false),
                                                        _shaderProgramIDTemp(0)
{
    _shaderProgramId = Divide::GLUtil::_invalidObjectID;

    _shaderStageTable[VERTEX_SHADER] = GL_VERTEX_SHADER;
    _shaderStageTable[FRAGMENT_SHADER] = GL_FRAGMENT_SHADER;
    _shaderStageTable[GEOMETRY_SHADER] = GL_GEOMETRY_SHADER;
    _shaderStageTable[TESSELATION_CTRL_SHADER] = GL_TESS_CONTROL_SHADER;
    _shaderStageTable[TESSELATION_EVAL_SHADER] = GL_TESS_EVALUATION_SHADER;
    _shaderStageTable[COMPUTE_SHADER] = GL_COMPUTE_SHADER;

    _shaderStage[VERTEX_SHADER] = nullptr;
    _shaderStage[FRAGMENT_SHADER] = nullptr;
    _shaderStage[GEOMETRY_SHADER] = nullptr;
    _shaderStage[TESSELATION_CTRL_SHADER] = nullptr;
    _shaderStage[TESSELATION_EVAL_SHADER] = nullptr;
    _shaderStage[COMPUTE_SHADER] = nullptr;

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
            DIVIDE_ASSERT(binary != NULL, "glShaderProgram error: could not allocate memory for the program binary!");
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
    DIVIDE_ASSERT(_threadedLoadComplete, "glShaderProgram error: tried to detach a shader from a program that didn't finish loading!");
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

        for (U8 i = 0; i < ShaderType_PLACEHOLDER; ++i){
            // Attach, compile and validate shaders into this program and update state
            attachShader(_shaderStage[i], _refreshStage[i]);
            _refreshStage[i] = false;
        }
        
        link();
    }

    validate();
    _shaderProgramId = _shaderProgramIDTemp;
    _textureSlots.clear();
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
    
    //NULL_SHADER shader means use shaderProgram(0)
    if (name.compare("NULL_SHADER") == 0){
        _validationQueued = false;
        _shaderProgramId = 0;
        _threadedLoadComplete = HardwareResource::generateHWResource(name);
        return true;
    }

    //Set the compilation state
    _linked = false;

    //check for refresh flags
    bool refresh = false;
    for(U8 i = 0; i < ShaderType_PLACEHOLDER; ++i){
        if (_refreshStage[i]){
            refresh = true;
            break;
        }
    }

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
        GLint lineCountOffset[ShaderType_PLACEHOLDER];
        std::string shaderSourceUniforms[ShaderType_PLACEHOLDER];

        GLint initialOffset = 16;
        
        if (GFX_DEVICE.getGPUVendor() == GPU_VENDOR_NVIDIA){ //nVidia specific
            initialOffset += 6;
        }

        std::string shaderSourceHeader;
        //get all of the preprocessor defines
        for (U8 i = 0; i < _definesList.size(); i++){
            if (_definesList[i].compare("DEFINE_PLACEHOLDER") == 0) continue;

            shaderSourceHeader.append("#define " + _definesList[i] + "\n" );
            initialOffset++;
        }

        for (U8 i = 0; i < ShaderType_PLACEHOLDER; ++i){
            lineCountOffset[i] = initialOffset;
            for (U8 j = 0; j < _customUniforms[i].size(); j++){
                shaderSourceUniforms[i].append("uniform " + _customUniforms[i][j] + ";\n");
                lineCountOffset[i]++;
            }
        }

        lineCountOffset[VERTEX_SHADER] += 8;
        lineCountOffset[FRAGMENT_SHADER] += 10;

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

        //Load the Vertex Shader
        std::string shaderCompileName[ShaderType_PLACEHOLDER];
        shaderCompileName[VERTEX_SHADER]   = shaderName + ".Vertex" + vertexProperties;
        shaderCompileName[FRAGMENT_SHADER] = shaderName + ".Fragment" + shaderProperties;
        shaderCompileName[GEOMETRY_SHADER] = shaderName + ".Geometry" + shaderProperties;
        shaderCompileName[TESSELATION_CTRL_SHADER] = shaderName + ".TessellationC" + shaderProperties;
        shaderCompileName[TESSELATION_EVAL_SHADER] = shaderName + ".TessellationE" + shaderProperties;
        shaderCompileName[COMPUTE_SHADER] = shaderName + ".Compute" + shaderProperties;

        for (U8 i = 0; i < ShaderType_PLACEHOLDER; ++i) {
            ShaderType type = (ShaderType)(i);

            //Recycle shaders only if we do not request a refresh for them
            if (!_refreshStage[type]) //See if it already exists in a compiled state
                _shaderStage[type] = ShaderManager::getInstance().findShader(shaderCompileName[type], refresh);
           
            //If it doesn't ...
            if (!_shaderStage[type]){
                //Use glsw to read the vertex part of the effect
                const char* sourceCode = glswGetShader(shaderCompileName[type].c_str(), lineCountOffset[type], _refreshStage[type]);
                if (sourceCode) {
                    //If reading was successful
                    std::string codeString(sourceCode);
                    Util::replaceStringInPlace(codeString, "//__CUSTOM_DEFINES__", shaderSourceHeader);
                    Util::replaceStringInPlace(codeString, "//__CUSTOM_UNIFORMS__", shaderSourceUniforms[type]);
                    //Load our shader and save it in the manager in case a new Shader Program needs it
                    _shaderStage[type] = ShaderManager::getInstance().loadShader(shaderCompileName[type], codeString, type, _refreshStage[type]);
                }
            }

            if (!_shaderStage[type]) PRINT_FN(Locale::get("WARN_GLSL_SHADER_LOAD"), shaderCompileName[type].c_str())
        }
    }

    return GFX_DEVICE.loadInContext(/*_threadedLoading ? GFX_LOADING_CONTEXT : */GFX_RENDERING_CONTEXT, DELEGATE_BIND(&glShaderProgram::threadedLoad, this, name));
}

///Cache uniform/attribute locations for shader programs
///When we call this function, we check our name<->address map to see if we queried the location before
///If we did not, ask the GPU to give us the variables address and save it for later use
GLint glShaderProgram::cachedLoc(const std::string& name, const bool uniform){
    //Not loaded or NULL_SHADER
    if (!isHWInitComplete() || _shaderProgramId == 0 || _shaderProgramId == Divide::GLUtil::_invalidObjectID) return -1;
    
    DIVIDE_ASSERT(_linked && _threadedLoadComplete, "glShaderProgram error: tried to query a shader program before linking!");
    const ShaderVarMap::const_iterator& it = _shaderVars.find(name);

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

    DIVIDE_ASSERT(_shaderProgramId != Divide::GLUtil::_invalidObjectID, "glShaderProgram error: tried to bind a shader program with an invalid handle!");

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
    DIVIDE_ASSERT(_bound && _linked, "glShaderProgram error: tried to set subroutines on an unbound or unlinked program!");
    if (!indices.empty() && indices[0] != GL_INVALID_INDEX)
        glUniformSubroutinesuiv(_shaderStageTable[type], (GLsizei)indices.size(), &indices.front());
    
}

void glShaderProgram::SetSubroutine(ShaderType type, U32 index) const {
    static U32 value[1];
    DIVIDE_ASSERT(_bound && _linked, "glShaderProgram error: tried to set subroutines on an unbound or unlinked program!");
    if (index != GL_INVALID_INDEX){
        value[0] = index;
        glUniformSubroutinesuiv(_shaderStageTable[type], 1, value);
    }
}

U32 glShaderProgram::GetSubroutineUniformCount(ShaderType type) const {
    DIVIDE_ASSERT(_linked, "glShaderProgram error: tried to query subroutines on an unlinked program!");
    
    I32 subroutineCount = 0;
    glGetProgramStageiv(_shaderProgramId, _shaderStageTable[type], GL_ACTIVE_SUBROUTINE_UNIFORMS, &subroutineCount);

    return std::max(subroutineCount, 0);
}

U32 glShaderProgram::GetSubroutineUniformIndex(ShaderType type, const std::string& name) const {
    DIVIDE_ASSERT(_linked, "glShaderProgram error: tried to query subroutines on an unlinked program!");

    return glGetSubroutineUniformLocation(_shaderProgramId, _shaderStageTable[type], name.c_str());
}

U32 glShaderProgram::GetSubroutineIndex(ShaderType type, const std::string& name) const {
    DIVIDE_ASSERT(_linked, "glShaderProgram error: tried to query subroutines on an unlinked program!");

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

void glShaderProgram::UniformTexture(GLint location, GLushort slot) {
    if (location == -1) return;
    if (!checkSlotUsage(location, slot)) return;

    if(!_bound) glProgramUniform1i(_shaderProgramId, location, slot);
    else        glUniform1i(location, slot);
}

bool glShaderProgram::checkSlotUsage(GLint location, GLushort slot) {
#ifdef _DEBUG
    const TextureSlotMap::const_iterator& it = _textureSlots.find(slot);
    if (it == _textureSlots.end() || it->second == location){
        _textureSlots[slot] = location;
        return true;
    }
    return false;
#else
    return true;
#endif
}