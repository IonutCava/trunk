#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glVertexArrayObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"

#define VOID_PTR_CAST(X) (const GLvoid*)(X)

bool glVertexArrayObject::_primitiveRestartEnabled = false;
GLuint glVertexArrayObject::_indexDelimiterCache = 0;

/// Default destructor
glVertexArrayObject::glVertexArrayObject(const PrimitiveType& type) : VertexBufferObject(type),
                                                                      _VAOid(0),
                                                                      _DepthVAOid(0),
                                                                      _refreshQueued(false),
                                                                      _animationData(false)
{
    //We assume everything is static draw
    _usage = GL_STATIC_DRAW;
}

glVertexArrayObject::~glVertexArrayObject()
{
    Destroy();
}

/// Delete buffer
void glVertexArrayObject::Destroy(){
    if(_IBOid > 0) 		 GLCheck(glDeleteBuffers(1, &_IBOid));
    if(_DepthVAOid > 0)  GLCheck(glDeleteVertexArrays(1, &_DepthVAOid));
    if(_DepthVBOid > 0)  GLCheck(glDeleteBuffers(1, &_DepthVBOid));
    if(_VBOid > 0)	     GLCheck(glDeleteBuffers(1, &_VBOid));
    if(_VAOid > 0)		 GLCheck(glDeleteVertexArrays(1, &_VAOid));
}

/// Create a dynamic or static VBO
bool glVertexArrayObject::Create(bool staticDraw){
    //If we wan't a dynamic buffer, than we are doing something outdated, such as software skinning, or software water rendering
    if(!staticDraw){
        GLenum usage = GL_DYNAMIC_DRAW;
        //OpenGLES support isn't added, but this check doesn't break anything, so I'll just leave it here -Ionut
        if(GFX_DEVICE.getApi() == OpenGLES) usage = GL_STREAM_DRAW;
        if(usage != _usage){
            _usage = usage;
            _refreshQueued = true;
        }
    }
    return true;
}

/// Update internal data based on construction method (dynamic/static)
bool glVertexArrayObject::Refresh(){
    //We can't have a VBO without vertex positions
    assert(!_dataPosition.empty());
    //If we do not use a shader, than most states are texCoord based, so a VAO is useless
    if(!_currentShader) ERROR_FN(Locale::get("ERROR_VAO_SHADER"));
    assert(_currentShader != nullptr);

    //Dynamic LOD elements (such as terrain) need dynamic indices
    //We can manually override indice usage (again, used by the Terrain rendering system)
    assert(!_hardwareIndicesL.empty() || !_hardwareIndicesS.empty());

    //Get the size of each data container
    ptrdiff_t nSizePosition    = _dataPosition.size()*sizeof(vec3<GLfloat>);
    ptrdiff_t nSizeColor       = _dataColor.size()*sizeof(vec3<GLubyte>);
    ptrdiff_t nSizeNormal      = _dataNormal.size()*sizeof(vec3<GLfloat>);
    ptrdiff_t nSizeTexcoord    = _dataTexcoord.size()*sizeof(vec2<GLfloat>);
    ptrdiff_t nSizeTangent     = _dataTangent.size()*sizeof(vec3<GLfloat>);
    ptrdiff_t nSizeBiTangent   = _dataBiTangent.size()*sizeof(vec3<GLfloat>);
    ptrdiff_t nSizeBoneWeights = _boneWeights.size()*sizeof(vec4<GLfloat>);
    ptrdiff_t nSizeBoneIndices = _boneIndices.size()*sizeof(vec4<GLubyte>);
    //Check if we have any animation data.
    _animationData = (!_boneWeights.empty() && !_boneIndices.empty());

    //get the 8-bit size factor of the VBO (nr. of bytes on Win32)
    ptrdiff_t vboSize = nSizePosition +
                        nSizeColor +
                        nSizeNormal +
                        nSizeTexcoord +
                        nSizeTangent +
                        nSizeBiTangent +
                        nSizeBoneWeights +
                        nSizeBoneIndices;

    computeTriangleList();
    //Depth optimization is not needed for small meshes. Use config.h to tweak this
    U32 triangleCount = _computeTriangles ? _dataTriangles.size() : Config::DEPTH_VBO_MIN_TRIANGLES + 1;
    D_PRINT_FN(Locale::get("DISPLAY_VBO_METRICS"), vboSize, Config::DEPTH_VBO_MIN_BYTES,_dataTriangles.size(),Config::DEPTH_VBO_MIN_TRIANGLES);
    if(triangleCount < Config::DEPTH_VBO_MIN_TRIANGLES || vboSize < Config::DEPTH_VBO_MIN_BYTES){
        _optimizeForDepth = false;
    }
    if(_forceOptimizeForDepth) _optimizeForDepth = true;

    //If we do not have a depth VAO or a depth VBO but we need one, create them
    if(_optimizeForDepth && (!_DepthVAOid && !_DepthVBOid)){
        // Create a VAO for depth rendering only if needed
        GLCheck(glGenVertexArrays(1, &_DepthVAOid));
        // Create a VBO for depth rendering only;
        GLCheck(glGenBuffers(1, &_DepthVBOid));
        if(!_DepthVAOid || !_DepthVBOid){
            ERROR_FN(Locale::get("ERROR_VBO_DEPTH_INIT"));
            // fallback to regular VAO rendering in depth-only mode
            _optimizeForDepth = false;
            _forceOptimizeForDepth = false;
        }
    }

    //Get Position offset
    _VBOoffsetPosition  = 0;
    ptrdiff_t previousOffset = _VBOoffsetPosition;
    ptrdiff_t previousCount = nSizePosition;
    //Also, get position offset for the DEPTH-only VBO.
    //No need to check if we want depth-only VBO's as it's a very small variable
    ptrdiff_t previousOffsetDEPTH = previousOffset;
    ptrdiff_t previousCountDEPTH = previousCount;
    //If we have and color data ...
    if(nSizeColor > 0) {
        _VBOoffsetColor = previousOffset + previousCount;
        previousOffset = _VBOoffsetColor;
        previousCount = nSizeColor;
    }
    //If we have any normal data ...
    if(nSizeNormal > 0){
        //... get it's offset
        _VBOoffsetNormal = previousOffset + previousCount;
        previousOffset = _VBOoffsetNormal;
        previousCount = nSizeNormal;
        //Also update our DEPTH-only offset
        if(_optimizeForDepth){
            //So far, depth and non-depth only offsets are equal
            previousOffsetDEPTH = previousOffset;
            previousCountDEPTH = previousCount;
        }
    }
    //We ignore all furture offsets for depth-only, until we reach the animation data
    if(nSizeTexcoord > 0){
        //Get tex coord's offsets
        _VBOoffsetTexcoord	= previousOffset + previousCount;
        previousOffset = _VBOoffsetTexcoord;
        previousCount = nSizeTexcoord;
    }

    if(nSizeTangent > 0){
        //tangent's offsets
        _VBOoffsetTangent	= previousOffset + previousCount;
        previousOffset = _VBOoffsetTangent;
        previousCount = nSizeTangent;
    }

    if(nSizeBiTangent > 0){
        //bitangent's offsets
        _VBOoffsetBiTangent	  = previousOffset + previousCount;
        previousOffset = _VBOoffsetBiTangent;
        previousCount = nSizeBiTangent;
    }

    // Bone indices and weights are always packed togheter to prevent invalid animations
    if(_animationData){
        //Get non depth-only offsets
        _VBOoffsetBoneWeights = previousOffset + previousCount;
        previousOffset = _VBOoffsetBoneWeights;
        previousCount   = nSizeBoneWeights;
        _VBOoffsetBoneIndices = previousOffset + previousCount;
        if(_optimizeForDepth){
            //And update depth-only updates if needed
            _VBOoffsetBoneWeightsDEPTH = previousOffsetDEPTH + previousCountDEPTH;
            previousOffsetDEPTH = _VBOoffsetBoneWeightsDEPTH;
            previousCountDEPTH = nSizeBoneWeights;
            _VBOoffsetBoneIndicesDEPTH = previousOffsetDEPTH + previousCountDEPTH;
        }
    }

    //Show an error and assert if the VAO creation failed
    if(_VAOid == 0)
        ERROR_FN(Locale::get("ERROR_VAO_INIT"));
    assert(_VAOid != 0);
    //Bind the current VAO to save our attribs
    GL_API::setActiveVAO(_VAOid);

    //Show an error and assert if we do not have a valid buffer for storage
    if(_VBOid == 0)
        PRINT_FN(Locale::get("ERROR_VBO_INIT"));
    assert(_VBOid != 0);
    //Bind the regular VBO
    GL_API::setActiveVBO(_VBOid);
    //Create our regular VBO with all of the data packed in it
    GLCheck(glBufferData(GL_ARRAY_BUFFER, vboSize, 0, _usage));
    //Add position data
    if(_positionDirty){
        GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetPosition, nSizePosition, VOID_PTR_CAST(&_dataPosition.front().x)));
        // Clear all update flags now, if we do not use a depth-only VBO, else clear it there
        if(!_optimizeForDepth) _positionDirty = false;
    }
    //Add color data
    if(_dataColor.size() && _colorDirty){
        GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetColor, nSizeColor, VOID_PTR_CAST(&_dataColor.front().r)));
        // Clear all update flags now, if we do not use a depth-only VBO, else clear it there
        _colorDirty = false;
    }
    //Add normal data
    if(_dataNormal.size() && _normalDirty){
        //Ignore for now
        /*for(U32 i = 0; i < _dataNormal.size(); i++){
            g_normalsSmall.push_back(UP1010102(_dataNormal[i].x,_dataNormal[i].z,_dataNormal[i].y,1.0f));
        }*/
        GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetNormal, nSizeNormal, VOID_PTR_CAST(&_dataNormal.front().x)));
        // Same rule applies for clearing the normal flag
        if(!_optimizeForDepth) _normalDirty = false;
    }
    //Add tex coord data
    if(_dataTexcoord.size() && _texcoordDirty){
        GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetTexcoord, nSizeTexcoord, VOID_PTR_CAST(&_dataTexcoord.front().s)));
        //We do not use tex coord data in our depth only vbo, so clear it here
        _texcoordDirty  = false;
    }
    //Add tangent data
    if(_dataTangent.size() && _tangentDirty){
        GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetTangent, nSizeTangent, VOID_PTR_CAST(&_dataTangent.front().x)));
        //We do not use tangent data in our depth only vbo, so clear it here
        _tangentDirty = false;
    }
    //Add bitangent data
    if(_dataBiTangent.size() && _bitangentDirty){
        GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBiTangent,nSizeBiTangent,VOID_PTR_CAST(&_dataBiTangent.front().x)));
        //We do not use bitangent data in our depth only vbo, so clear it here
        _bitangentDirty = false;
    }
    //Add animation data
    if(_animationData) {
        if(_weightsDirty){
            GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneWeights,nSizeBoneWeights, VOID_PTR_CAST(&_boneWeights.front().x)));
            //Clear the weights flag after we updated the depth-only vbo as well
            if(!_optimizeForDepth) _weightsDirty   = false;
        }

        if(_indicesDirty){
            GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneIndices,nSizeBoneIndices,VOID_PTR_CAST(&_boneIndices.front().x)));
            //Clear the indice flag after we updated the depth-only vbo as well
            if(!_optimizeForDepth) _indicesDirty   = false;
        }
    }

    //Show an error and assert if the IBO creation failed
    if(_IBOid == 0) PRINT_FN(Locale::get("ERROR_IBO_INIT"));
    assert(_IBOid != 0);
    //Bind and create our IBO
    GLCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBOid));
    if(_largeIndices){
        GLCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER, _hardwareIndicesL.size() * sizeof(GLuint),
                         VOID_PTR_CAST(&_hardwareIndicesL.front()), GL_STATIC_DRAW));
    }else{
        GLCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER, _hardwareIndicesS.size() * sizeof(GLushort),
                         VOID_PTR_CAST(&_hardwareIndicesS.front()), GL_STATIC_DRAW));
    }

    Upload_VBO_Attributes();

    //We need to make double sure, we are not trying to create a regular VBO during a depth pass or else everything will crash and burn
    bool oldDepthState = _depthPass;
    _depthPass = false;

    //Time for our depth-only VBO creation
    if(_optimizeForDepth){
        // Make sure we bind the correct attribs
        _depthPass = true;

        //Show an error and assert if the depth VAO isn't created properly
        if(_DepthVAOid == 0) PRINT_FN(Locale::get("ERROR_VAO_DEPTH_INIT"));
        assert(_DepthVAOid != 0);
       //Bind the depth-only VAO if we are using one
        GL_API::setActiveVAO(_DepthVAOid);

        //Show an error and assert if we do not have a valid buffer to stare data in
        if(_DepthVBOid == 0) PRINT_FN(Locale::get("ERROR_VBO_DEPTH_INIT"));
        assert(_DepthVBOid != 0);
        //Bind our depth-only buffer
        GL_API::setActiveVBO(_DepthVBOid);
        //Create our depth-only VBO with all of the data packed in it
        GLCheck(glBufferData(GL_ARRAY_BUFFER, nSizePosition+
                                              nSizeNormal+
                                              nSizeBoneWeights+
                                              nSizeBoneIndices,
                                              0, _usage));
        //Add position data
        if(_positionDirty){
            GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetPosition, nSizePosition,	VOID_PTR_CAST(&_dataPosition.front().x)));
            // Clear the position update flag now
            _positionDirty  = false;
        }
        //Add normal data
        if(_dataNormal.size() && _normalDirty){
            //Ignore for now
            /*for(U32 i = 0; i < _dataNormal.size(); i++){
                g_normalsSmall.push_back(UP1010102(_dataNormal[i].x,_dataNormal[i].z,_dataNormal[i].y,1.0f));
            }*/
            GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetNormal, nSizeNormal, VOID_PTR_CAST(&_dataNormal.front().x)));
            // Clear the normal update flag now
            _normalDirty    = false;
        }
        //Add animation data
        if(_animationData) {
            if(_indicesDirty){
                GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneIndicesDEPTH, nSizeBoneIndices, VOID_PTR_CAST(&_boneIndices.front().x)));
                _indicesDirty   = false;
            }

            if(_weightsDirty){
                GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneWeightsDEPTH, nSizeBoneWeights, VOID_PTR_CAST(&_boneWeights.front().x)));
                _weightsDirty   = false;
            }
        }

        assert(_IBOid != 0);
        GLCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBOid));

        Upload_VBO_Depth_Attributes();
    }
    //Restore depth state to it's previous value
    _depthPass = oldDepthState;

    checkStatus();

    _refreshQueued = false;
    return true;
}

/// This method creates the initial VBO
/// The only diferrence between Create and Refresh is the generation of a new buffer
bool glVertexArrayObject::CreateInternal() {
    if(_dataPosition.empty()){
        ERROR_FN(Locale::get("ERROR_VBO_POSITION"));
        _created = false;
        return _created;
    }

    // Enforce all update flags. Kinda useless, but it doesn't hurt
    _positionDirty  = _colorDirty = true;
    _normalDirty    = _texcoordDirty = true;
    _tangentDirty   = _bitangentDirty = true;
    _indicesDirty   = _weightsDirty   = true;

    // Generate a "Vertex Array Object"
    GLCheck(glGenVertexArrays(1, &_VAOid));
    // Generate a new Vertex Buffer Object
    GLCheck(glGenBuffers(1, &_VBOid));
    // Validate buffer creation
    if(!_VAOid || !_VBOid) {
        ERROR_FN(Locale::get(_VAOid > 0 ? "ERROR_VBO_INIT" : "ERROR_VAO_INIT"));
        _created = false;
        return _created;
    }
    // Generate an "Index Buffer Object"
    GLCheck(glGenBuffers(1, &_IBOid));
    if(!_IBOid){
        ERROR_FN(Locale::get("ERROR_IBO_INIT"));
        _created = false;
        return _created;
    }
    
    // calling refresh updates all stored information and sends it to the GPU
    _created = queueRefresh();
    return _created;
}

/// Inform the VBO what shader we use to render the object so that the data is uploaded correctly
/// All shaders must use the same attribute locations!!!
void glVertexArrayObject::setShaderProgram(ShaderProgram* const shaderProgram) {
    if(!shaderProgram || shaderProgram->getId() == 0/*NULL_SHADER*/){
        ERROR_FN(Locale::get("ERROR_VAO_SHADER_USER"));
    }
    assert(shaderProgram != nullptr);
    _currentShader = shaderProgram;
}

///This draw method does not need an Index Buffer!
void glVertexArrayObject::DrawRange(){
    //SetActive returns immediately if it's already been called
    if(!SetActive()) return;

    void* offset = BUFFER_OFFSET(_firstElement * (_largeIndices ? sizeof(GLuint) : sizeof(GLushort)));

    GLCheck(glDrawElements(glPrimitiveTypeTable[_type], _rangeCount, glDataFormat[_format], offset));
}

///This draw method needs an Index Buffer!
void glVertexArrayObject::Draw(const U8 LODindex){
    if(!SetActive()) return;

    U32 lod = std::min((U32)LODindex, _indiceLimits.size() - 1);
    GLsizei count = (GLsizei)(_largeIndices ? _hardwareIndicesL.size() : _hardwareIndicesS.size());

    assert(_indiceLimits[lod].y > _indiceLimits[lod].x && _indiceLimits[lod].y < (GLuint)count);

    GLCheck(glDrawRangeElements(glPrimitiveTypeTable[_type], _indiceLimits[lod].x, _indiceLimits[lod].y, count, glDataFormat[_format], BUFFER_OFFSET(0)));
 }

/// If we do not have a VAO object, we use vertex arrays as fail safes
bool glVertexArrayObject::SetActive(){
    //if the shader is not ready, do not draw anything
    if(!_currentShader->isBound() || GL_API::getActiveProgram()->getId() != _currentShader->getId())
        return false;
  
    // Make sure we have valid data
    if(!_created) CreateInternal();
    // Make sure we do not have a refresh request queued up
    if(_refreshQueued) Refresh();
    //Choose optimal VAO/VBO combo
    setDepthPass(GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE));

    bool enableState = false;
    if(_depthPass && _optimizeForDepth)
        enableState = GL_API::setActiveVAO(_DepthVAOid);
    else 		                     
        enableState = GL_API::setActiveVAO(_VAOid);

    if(enableState){
        //Use primitive restart if needed
        if(_indexDelimiter != 0){
            if(_indexDelimiter != _indexDelimiterCache){
                GLCheck(glPrimitiveRestartIndex(_indexDelimiter));
            }
            if(!_primitiveRestartEnabled){
                GLCheck(glEnable(GL_PRIMITIVE_RESTART));
                _primitiveRestartEnabled = true;
            }
        }else{//index delimiter is zero
            if(_primitiveRestartEnabled){
                GLCheck(glDisable(GL_PRIMITIVE_RESTART));
                _primitiveRestartEnabled = false;
            }
        }

        _indexDelimiterCache = _indexDelimiter;
    }

    return true;
}

//Bind vertex attributes (only vertex, normal and animation data is used for the depth-only vbo)
void glVertexArrayObject::Upload_VBO_Attributes(){
    GLCheck(glEnableVertexAttribArray(Divide::VERTEX_POSITION_LOCATION));
    GLCheck(glVertexAttribPointer(Divide::VERTEX_POSITION_LOCATION,
                                  3,
                                  GL_FLOAT, GL_FALSE,
                                  sizeof(vec3<GLfloat>),
                                  BUFFER_OFFSET(_VBOoffsetPosition)));
    if(!_dataColor.empty()) {
        GLCheck(glEnableVertexAttribArray(Divide::VERTEX_COLOR_LOCATION));
        GLCheck(glVertexAttribPointer(Divide::VERTEX_COLOR_LOCATION,
                                      3,
                                      GL_UNSIGNED_BYTE, GL_FALSE,
                                      sizeof(vec3<GLubyte>),
                                      BUFFER_OFFSET(_VBOoffsetColor)));
    }
    if(!_dataNormal.empty()) {
        GLCheck(glEnableVertexAttribArray(Divide::VERTEX_NORMAL_LOCATION));
        GLCheck(glVertexAttribPointer(Divide::VERTEX_NORMAL_LOCATION,
                                      3,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBOoffsetNormal)));
    }

    if(!_dataTexcoord.empty()) {
        GLCheck(glEnableVertexAttribArray(Divide::VERTEX_TEXCOORD_LOCATION));
        GLCheck(glVertexAttribPointer(Divide::VERTEX_TEXCOORD_LOCATION,
                                      2,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec2<GLfloat>),
                                      BUFFER_OFFSET(_VBOoffsetTexcoord)));
    }

    if(!_dataTangent.empty()){
        GLCheck(glEnableVertexAttribArray(Divide::VERTEX_TANGENT_LOCATION));
        GLCheck(glVertexAttribPointer(Divide::VERTEX_TANGENT_LOCATION,
                                      3,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBOoffsetTangent)));
    }

    if(!_dataBiTangent.empty()){
        GLCheck(glEnableVertexAttribArray(Divide::VERTEX_BITANGENT_LOCATION));
        GLCheck(glVertexAttribPointer(Divide::VERTEX_BITANGENT_LOCATION,
                                      3,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBOoffsetBiTangent)));
    }

    if(_animationData){
        //Bone weights
        GLCheck(glEnableVertexAttribArray(Divide::VERTEX_BONE_WEIGHT_LOCATION));
        GLCheck(glVertexAttribPointer(Divide::VERTEX_BONE_WEIGHT_LOCATION,
                                      4,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec4<GLfloat>),
                                      BUFFER_OFFSET(_VBOoffsetBoneWeights)));

        // Bone indices
        GLCheck(glEnableVertexAttribArray(Divide::VERTEX_BONE_INDICE_LOCATION));
        GLCheck(glVertexAttribIPointer(Divide::VERTEX_BONE_INDICE_LOCATION,
                                       4,
                                       GL_UNSIGNED_BYTE,
                                       sizeof(vec4<GLubyte>),
                                       BUFFER_OFFSET(_VBOoffsetBoneIndices)));
    }
}

void glVertexArrayObject::Upload_VBO_Depth_Attributes(){
    GLCheck(glEnableVertexAttribArray(Divide::VERTEX_POSITION_LOCATION));
    GLCheck(glVertexAttribPointer(Divide::VERTEX_POSITION_LOCATION,
                                  3,
                                  GL_FLOAT, GL_FALSE,
                                  sizeof(vec3<GLfloat>),
                                  BUFFER_OFFSET(_VBOoffsetPosition)));
    if(!_dataNormal.empty()) {
        GLCheck(glEnableVertexAttribArray(Divide::VERTEX_NORMAL_LOCATION));
        GLCheck(glVertexAttribPointer(Divide::VERTEX_NORMAL_LOCATION,
                                      3,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBOoffsetNormal)));
    }

    if(_animationData){
        //Bone weights
        GLCheck(glEnableVertexAttribArray(Divide::VERTEX_BONE_WEIGHT_LOCATION));
        GLCheck(glVertexAttribPointer(Divide::VERTEX_BONE_WEIGHT_LOCATION,
                                      4,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec4<GLfloat>),
                                      BUFFER_OFFSET(_VBOoffsetBoneWeightsDEPTH)));

        // Bone indices
        GLCheck(glEnableVertexAttribArray(Divide::VERTEX_BONE_INDICE_LOCATION));
        GLCheck(glVertexAttribIPointer(Divide::VERTEX_BONE_INDICE_LOCATION,
                                       4,
                                       GL_UNSIGNED_BYTE,
                                       sizeof(vec4<GLubyte>),
                                       BUFFER_OFFSET(_VBOoffsetBoneIndicesDEPTH)));
    }
}

inline bool degenerateTriangleMatch(const vec3<U32>& triangle){
    return (triangle.x != triangle.y && triangle.x != triangle.z && triangle.y != triangle.z);
}

//Create a list of triangles from the vertices + indices lists based on primitive type
bool glVertexArrayObject::computeTriangleList(){
    if(!_computeTriangles) return true;
    if(!_dataTriangles.empty()) return true;
    if(_hardwareIndicesL.empty() && _hardwareIndicesS.empty()) return false;

    U32 indiceCount = _largeIndices ? _hardwareIndicesL.size() : _hardwareIndicesS.size();

    if(_type == TRIANGLE_STRIP){
        vec3<U32> curTriangle;
        _dataTriangles.reserve(indiceCount * 0.5);
        if(_largeIndices){
            for(U32 i = 2; i < indiceCount; i++){
                curTriangle.set(_hardwareIndicesL[i - 2], _hardwareIndicesL[i - 1], _hardwareIndicesL[i]);
                //Check for correct winding
                if (i % 2 != 0) std::swap(curTriangle.y, curTriangle.z);

                _dataTriangles.push_back(curTriangle);
            }
        }else{
            for(U32 i = 2; i < indiceCount; i++){
                curTriangle.set(_hardwareIndicesS[i - 2], _hardwareIndicesS[i - 1], _hardwareIndicesS[i]);
                //Check for correct winding
                if (i % 2 != 0) std::swap(curTriangle.y, curTriangle.z);

                _dataTriangles.push_back(curTriangle);
            }
        }
    }else if (_type == TRIANGLES){
        indiceCount /= 3;
        _dataTriangles.reserve(indiceCount);
        if(_largeIndices){
            for(U32 i = 0; i < indiceCount; i++){
                 _dataTriangles.push_back(vec3<U32>(_hardwareIndicesL[i*3 + 0],_hardwareIndicesL[i*3 + 1],_hardwareIndicesL[i*3 + 2]));
            }
         }else{
             for(U32 i = 0; i < indiceCount; i++){
                 _dataTriangles.push_back(vec3<U32>(_hardwareIndicesS[i*3 + 0],_hardwareIndicesS[i*3 + 1],_hardwareIndicesS[i*3 + 2]));
            }
        }
    }/*else if (_type == QUADS){
        indiceCount *= 0.5;
        _dataTriangles.reserve(indiceCount);
        if(_largeIndices){
            for(U32 i = 0; i < (indiceCount) - 2; i++){
                if (!(_hardwareIndicesL[i] % 2)){
                    _dataTriangles.push_back(vec3<U32>(_hardwareIndicesL[i*2 + 0],_hardwareIndicesL[i*2 + 1],_hardwareIndicesL[i*2 + 3]));
                }else{
                    _dataTriangles.push_back(vec3<U32>(_hardwareIndicesL[i*2 + 3],_hardwareIndicesL[i*2 + 1],_hardwareIndicesL[i*2 + 0]));
                }
            }
        }else{
                for(U32 i = 0; i < (indiceCount) - 2; i++){
                if (!(_hardwareIndicesS[i] % 2)){
                    _dataTriangles.push_back(vec3<U32>(_hardwareIndicesS[i*2 + 0],_hardwareIndicesS[i*2 + 1],_hardwareIndicesS[i*2 + 3]));
                }else{
                    _dataTriangles.push_back(vec3<U32>(_hardwareIndicesS[i*2 + 3],_hardwareIndicesS[i*2 + 1],_hardwareIndicesS[i*2 + 0]));
                }
            }
        }
    }*/

    //Check for degenerate triangles
    _dataTriangles.erase(std::partition(_dataTriangles.begin(), _dataTriangles.end(), degenerateTriangleMatch), _dataTriangles.end());

    assert(_dataTriangles.size() > 0);
    return true;
}
//Various post-create checks
void glVertexArrayObject::checkStatus(){
}