#include "Headers/glVertexArray.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/ShaderManager.h"

#define VOID_PTR_CAST(X) (const GLvoid*)(X)

/// Default destructor
glVertexArray::glVertexArray(const PrimitiveType& type) : VertexBuffer(type),
                                                          _refreshQueued(false),
                                                          _animationData(false),
                                                          _formatInternal(GL_NONE),
                                                          _typeInternal(GL_NONE)
{
    //We assume everything is static draw
    _usage = GL_STATIC_DRAW;
    _prevSizePosition = -1;
    _prevSizeColor = -1;
    _prevSizeNormal = -1;
    _prevSizeTexcoord = -1;
    _prevSizeTangent = -1;
    _prevSizeBiTangent = -1;
    _prevSizeBoneWeights = -1;
    _prevSizeBoneIndices = -1;
    _prevSizeIndices = -1;
    _VAOid = _DepthVAOid = _VBid = _DepthVBid = _IBid = 0;

    _multiCount.reserve(MAX_DRAW_COMMANDS);
    _multiIndices.reserve(MAX_DRAW_COMMANDS);
}

glVertexArray::~glVertexArray()
{
    Destroy();
}

/// Delete buffer
void glVertexArray::Destroy(){
    if(_IBid > 0) 		 glDeleteBuffers(1, &_IBid);
    if(_DepthVAOid > 0)  glDeleteVertexArrays(1, &_DepthVAOid);
    if(_DepthVBid > 0)   glDeleteBuffers(1, &_DepthVBid);
    if(_VBid > 0)	     glDeleteBuffers(1, &_VBid);
    if(_VAOid > 0)		 glDeleteVertexArrays(1, &_VAOid);
    _VAOid = _DepthVAOid = _VBid = _DepthVBid = _IBid = 0;
}

/// Create a dynamic or static VB
bool glVertexArray::Create(bool staticDraw){
    //If we want a dynamic buffer, then we are doing something outdated, such as software skinning, or software water rendering
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
bool glVertexArray::Refresh(){
    //If we do not use a shader, than most states are texCoord based, so a VAO is useless
    if(!_currentShader) ERROR_FN(Locale::get("ERROR_VAO_SHADER"));
    assert(_currentShader != nullptr);

    if (_currentShader->getGUID() == ShaderManager::getInstance().getDefaultShader()->getGUID())
        return false;

    //Dynamic LOD elements (such as terrain) need dynamic indices
    //We can manually override indice usage (again, used by the Terrain rendering system)
    assert(!_hardwareIndicesL.empty() || !_hardwareIndicesS.empty());

    //Get the size of each data container
    GLsizei nSizePosition    = (GLsizei)(_dataPosition.size()*sizeof(vec3<GLfloat>));
    GLsizei nSizeColor       = (GLsizei)(_dataColor.size()*sizeof(vec3<GLubyte>));
    GLsizei nSizeNormal      = (GLsizei)(_dataNormal.size()*sizeof(vec3<GLfloat>));
    GLsizei nSizeTexcoord    = (GLsizei)(_dataTexcoord.size()*sizeof(vec2<GLfloat>));
    GLsizei nSizeTangent     = (GLsizei)(_dataTangent.size()*sizeof(vec3<GLfloat>));
    GLsizei nSizeBiTangent   = (GLsizei)(_dataBiTangent.size()*sizeof(vec3<GLfloat>));
    GLsizei nSizeBoneWeights = (GLsizei)(_boneWeights.size()*sizeof(vec4<GLfloat>));
    GLsizei nSizeBoneIndices = (GLsizei)(_boneIndices.size()*sizeof(vec4<GLubyte>));
    GLsizei nSizeIndices     = (GLsizei)(_largeIndices ? _hardwareIndicesL.size() * sizeof(GLuint) : _hardwareIndicesS.size() * sizeof(GLushort));
    // if any of the VBO's components changed size, we need to recreate the entire buffer.
    // could be optimized, I know, but ... 
    bool sizeChanged = ((nSizePosition != _prevSizePosition) || 
                        (nSizeColor != _prevSizeColor) ||
                        (nSizeNormal != _prevSizeNormal) ||
                        (nSizeTexcoord != _prevSizeTexcoord) ||
                        (nSizeTangent != _prevSizeTangent) ||
                        (nSizeBiTangent != _prevSizeBiTangent) ||
                        (nSizeBoneWeights != _prevSizeBoneWeights) ||
                        (nSizeBoneIndices != _prevSizeBoneIndices));

    bool indicesChanged = (nSizeIndices != _prevSizeIndices);

    if (sizeChanged){
        _prevSizePosition = nSizePosition;
        _prevSizeColor = nSizeColor;
        _prevSizeNormal = nSizeNormal;
        _prevSizeTexcoord = nSizeTexcoord;
        _prevSizeTangent = nSizeTangent;
        _prevSizeBiTangent = nSizeBiTangent;
        _prevSizeBoneWeights = nSizeBoneWeights;
        _prevSizeBoneIndices = nSizeBoneIndices;
    }
    if (indicesChanged){
        _prevSizeIndices = nSizeIndices;
    }

    if(!(_positionDirty || _colorDirty || _normalDirty || _texcoordDirty || _tangentDirty || _bitangentDirty || _weightsDirty || _indicesDirty || sizeChanged || indicesChanged)){
        _refreshQueued = false;
        return true;
    }

    //Check if we have any animation data.
    _animationData = (!_boneWeights.empty() && !_boneIndices.empty());

    //get the 8-bit size factor of the VB
    GLsizei vbSize = nSizePosition + nSizeColor + nSizeNormal + nSizeTexcoord + nSizeTangent + nSizeBiTangent + nSizeBoneWeights + nSizeBoneIndices;

    if (_VAOid != 0 || sizeChanged || indicesChanged) computeTriangleList();
    //Depth optimization is not needed for small meshes. Use config.h to tweak this
    GLsizei triangleCount = (GLsizei)(_computeTriangles ? _dataTriangles.size() : Config::DEPTH_VB_MIN_TRIANGLES + 1);
    D_PRINT_FN(Locale::get("DISPLAY_VB_METRICS"), vbSize, Config::DEPTH_VB_MIN_BYTES,_dataTriangles.size(),Config::DEPTH_VB_MIN_TRIANGLES);
    if(triangleCount < Config::DEPTH_VB_MIN_TRIANGLES || vbSize < Config::DEPTH_VB_MIN_BYTES){
        _optimizeForDepth = false;
    }

    if(_forceOptimizeForDepth) _optimizeForDepth = true;

    //If we do not have a depth VAO or a depth VB but we need one, create them
    if(_optimizeForDepth && !_DepthVAOid){
        // Create a VAO for depth rendering only if needed
        glGenVertexArrays(1, &_DepthVAOid);
        // Create a VB for depth rendering only;
        glGenBuffers(1, &_DepthVBid);
        if(!_DepthVAOid || !_DepthVBid){
            ERROR_FN(Locale::get("ERROR_VB_DEPTH_INIT"));
            // fall back to regular VAO rendering in depth-only mode
            _optimizeForDepth = false;
            _forceOptimizeForDepth = false;
        }
    }

    //Get Position offset
    _VBoffsetPosition  = 0;
    GLsizei previousOffset = _VBoffsetPosition;
    GLsizei previousCount  = nSizePosition;
    //Also, get position offset for the DEPTH-only VB.
    //No need to check if we want depth-only VB's as it's a very small variable
    GLsizei previousOffsetDEPTH = previousOffset;
    GLsizei previousCountDEPTH  = previousCount;
    //If we have and color data ...
    if(nSizeColor > 0) {
        _VBoffsetColor = previousOffset + previousCount;
        previousOffset = _VBoffsetColor;
        previousCount = nSizeColor;
    }
    //If we have any normal data ...
    if(nSizeNormal > 0){
        //... get it's offset
        _VBoffsetNormal = previousOffset + previousCount;
        previousOffset = _VBoffsetNormal;
        previousCount = nSizeNormal;
        //Also update our DEPTH-only offset
        if(_optimizeForDepth){
            //So far, depth and non-depth only offsets are equal
            previousOffsetDEPTH = previousOffset;
            previousCountDEPTH = previousCount;
        }
    }
    //We ignore all further offsets for depth-only, until we reach the animation data
    if(nSizeTexcoord > 0){
        //Get texCoord's offsets
        _VBoffsetTexcoord	= previousOffset + previousCount;
        previousOffset = _VBoffsetTexcoord;
        previousCount = nSizeTexcoord;
    }

    if(nSizeTangent > 0){
        //Tangent's offsets
        _VBoffsetTangent	= previousOffset + previousCount;
        previousOffset = _VBoffsetTangent;
        previousCount = nSizeTangent;
    }

    if(nSizeBiTangent > 0){
        //BiTangent's offsets
        _VBoffsetBiTangent	  = previousOffset + previousCount;
        previousOffset = _VBoffsetBiTangent;
        previousCount = nSizeBiTangent;
    }

    // Bone indices and weights are always packed together to prevent invalid animations
    if(_animationData){
        //Get non depth-only offsets
        _VBoffsetBoneWeights = previousOffset + previousCount;
        previousOffset = _VBoffsetBoneWeights;
        previousCount   = nSizeBoneWeights;
        _VBoffsetBoneIndices = previousOffset + previousCount;
        if(_optimizeForDepth){
            //And update depth-only updates if needed
            _VBoffsetBoneWeightsDEPTH = previousOffsetDEPTH + previousCountDEPTH;
            previousOffsetDEPTH = _VBoffsetBoneWeightsDEPTH;
            previousCountDEPTH = nSizeBoneWeights;
            _VBoffsetBoneIndicesDEPTH = previousOffsetDEPTH + previousCountDEPTH;
        }
    }

    //Show an error and assert if the VAO creation failed
    if(_VAOid == 0) ERROR_FN(Locale::get("ERROR_VAO_INIT"));
    assert(_VAOid != 0);
    //Show an error and assert if we do not have a valid buffer for storage
    if (_VBid == 0)    PRINT_FN(Locale::get("ERROR_VB_INIT"));
    assert(_VBid != 0);

    //Bind the current VAO to save our attribs
    GL_API::setActiveVAO(_VAOid);
    //Bind the regular VB
    GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _VBid);

    if (sizeChanged){
        //Create our regular VB with all of the data packed in it
        glBufferData(GL_ARRAY_BUFFER, vbSize, NULL, _usage);
        _positionDirty = _colorDirty = _normalDirty = _texcoordDirty = _tangentDirty = _bitangentDirty = _weightsDirty = _indicesDirty = true;
    }
    //Add position data
    if(_positionDirty){
        glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetPosition, nSizePosition, VOID_PTR_CAST(&_dataPosition.front().x));
    }
    //Add color data
    if(_dataColor.size() && _colorDirty){
        glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetColor, nSizeColor, VOID_PTR_CAST(&_dataColor.front().r));
    }
    //Add normal data
    if(_dataNormal.size() && _normalDirty){
        //Ignore for now
        /*for(U32 i = 0; i < _dataNormal.size(); i++){
            g_normalsSmall.push_back(UP1010102(_dataNormal[i].x,_dataNormal[i].z,_dataNormal[i].y,1.0f));
        }*/
        glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetNormal, nSizeNormal, VOID_PTR_CAST(&_dataNormal.front().x));
    }
    //Add tex coord data
    if(_dataTexcoord.size() && _texcoordDirty){
        glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetTexcoord, nSizeTexcoord, VOID_PTR_CAST(&_dataTexcoord.front().s));
    }
    //Add tangent data
    if(_dataTangent.size() && _tangentDirty){
        glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetTangent, nSizeTangent, VOID_PTR_CAST(&_dataTangent.front().x));
    }
    //Add bitangent data
    if(_dataBiTangent.size() && _bitangentDirty){
        glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetBiTangent,nSizeBiTangent,VOID_PTR_CAST(&_dataBiTangent.front().x));
    }
    //Add animation data
    if(_animationData) {
        if(_weightsDirty) glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetBoneWeights,nSizeBoneWeights, VOID_PTR_CAST(&_boneWeights.front().x));
        if(_indicesDirty) glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetBoneIndices,nSizeBoneIndices,VOID_PTR_CAST(&_boneIndices.front().x));
    }

    if (indicesChanged){
        //Show an error and assert if the IB creation failed
        if(_IBid == 0) PRINT_FN(Locale::get("ERROR_IB_INIT"));
        assert(_IBid != 0);
        //Bind and create our IB
        GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBid);
        if(_largeIndices) glBufferData(GL_ELEMENT_ARRAY_BUFFER, _hardwareIndicesL.size() * sizeof(GLuint), VOID_PTR_CAST(&_hardwareIndicesL.front()), GL_STATIC_DRAW);
        else              glBufferData(GL_ELEMENT_ARRAY_BUFFER, _hardwareIndicesS.size() * sizeof(GLushort), VOID_PTR_CAST(&_hardwareIndicesS.front()), GL_STATIC_DRAW);
    }

    Upload_VB_Attributes();

    //We need to make double sure, we are not trying to create a regular VB during a depth pass or else everything will crash and burn
    bool oldDepthState = _depthPass;
    _depthPass = false;

    //Time for our depth-only VB creation
    if(_optimizeForDepth){
        // Make sure we bind the correct attribs
        _depthPass = true;

        //Show an error and assert if the depth VAO isn't created properly
        if(_DepthVAOid == 0) PRINT_FN(Locale::get("ERROR_VAO_DEPTH_INIT"));
        assert(_DepthVAOid != 0);
       //Bind the depth-only VAO if we are using one
        GL_API::setActiveVAO(_DepthVAOid);

        //Show an error and assert if we do not have a valid buffer to stare data in
        if(_DepthVBid == 0) PRINT_FN(Locale::get("ERROR_VB_DEPTH_INIT"));
        assert(_DepthVBid != 0);
        //Bind our depth-only buffer
        GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _DepthVBid);
        if (sizeChanged){
            //Create our depth-only VB with all of the data packed in it
            glBufferData(GL_ARRAY_BUFFER, nSizePosition + nSizeNormal + nSizeBoneWeights + nSizeBoneIndices, NULL, _usage);
        }
        //Add position data
        if(_positionDirty){
            glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetPosition, nSizePosition,	VOID_PTR_CAST(&_dataPosition.front().x));
        }
        //Add normal data
        if(_dataNormal.size() && _normalDirty){
            //Ignore for now
            /*for(U32 i = 0; i < _dataNormal.size(); i++){
                g_normalsSmall.push_back(UP1010102(_dataNormal[i].x,_dataNormal[i].z,_dataNormal[i].y,1.0f));
            }*/
            glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetNormal, nSizeNormal, VOID_PTR_CAST(&_dataNormal.front().x));
        }
        //Add animation data
        if(_animationData) {
            if(_indicesDirty) glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetBoneIndicesDEPTH, nSizeBoneIndices, VOID_PTR_CAST(&_boneIndices.front().x));
            if(_weightsDirty) glBufferSubData(GL_ARRAY_BUFFER, _VBoffsetBoneWeightsDEPTH, nSizeBoneWeights, VOID_PTR_CAST(&_boneWeights.front().x));
        }

        assert(_IBid != 0);
        GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBid);

        Upload_VB_Depth_Attributes();
    }
    //Restore depth state to it's previous value
    _depthPass = oldDepthState;
    _formatInternal = glDataFormat[_format];
    _typeInternal = glPrimitiveTypeTable[_type];

    _positionDirty = _colorDirty = _normalDirty = _texcoordDirty = _tangentDirty = _bitangentDirty = _weightsDirty = _indicesDirty = false;

    checkStatus();

    _refreshQueued = false;
    return true;
}

/// This method creates the initial VB
/// The only difference between Create and Refresh is the generation of a new buffer
bool glVertexArray::CreateInternal() {
    if(_dataPosition.empty()){
        ERROR_FN(Locale::get("ERROR_VB_POSITION"));
        _created = false;
        return _created;
    }

    // Enforce all update flags. Kinda useless, but it doesn't hurt
    _positionDirty  = _colorDirty     = true;
    _normalDirty    = _texcoordDirty  = true;
    _tangentDirty   = _bitangentDirty = true;
    _indicesDirty   = _weightsDirty   = true;

    // Generate a "Vertex Array Object"
    glGenVertexArrays(1, &_VAOid);
    // Generate a new Vertex Buffer Object
    glGenBuffers(1, &_VBid);
    // Validate buffer creation
    if(!_VAOid || !_VBid) {
        ERROR_FN(Locale::get(_VAOid > 0 ? "ERROR_VB_INIT" : "ERROR_VAO_INIT"));
        _created = false;
        return _created;
    }
    // Generate an "Index Buffer Object"
    glGenBuffers(1, &_IBid);
    if(!_IBid){
        ERROR_FN(Locale::get("ERROR_IB_INIT"));
        _created = false;
        return _created;
    }
    
    // calling refresh updates all stored information and sends it to the GPU
    _created = queueRefresh();
    return _created;
}

/// Inform the VB what shader we use to render the object so that the data is uploaded correctly
/// All shaders must use the same attribute locations!!!
void glVertexArray::setShaderProgram(ShaderProgram* const shaderProgram) {
    if(!shaderProgram || shaderProgram->getId() == 0/*NULL_SHADER*/){
        ERROR_FN(Locale::get("ERROR_VAO_SHADER_USER"));
    }
    assert(shaderProgram != nullptr);
    _currentShader = shaderProgram;
}

void glVertexArray::DrawCommands(const vectorImpl<DeferredDrawCommand>& commands, bool skipBind) {
    //SetActive returns immediately if it's already been called
    if (!skipBind)
        if (!SetActive()) return;
   
   assert(commands.size() < MAX_DRAW_COMMANDS);
   _multiCount.resize(0);
   _multiIndices.resize(0);
   GLsizei size = (_largeIndices ? sizeof(GLuint) : sizeof(GLushort));
   for (const DeferredDrawCommand& cmd : commands){
       _multiCount.push_back(cmd._cmd.count);
       _multiIndices.push_back(VOID_PTR_CAST(cmd._cmd.firstIndex * size));
   }

   glMultiDrawElements(_typeInternal, &_multiCount.front(), _formatInternal, &_multiIndices.front(), (GLsizei)commands.size());
   GL_API::registerDrawCall();
}

///This draw method does not need an Index Buffer!
void glVertexArray::DrawRange(bool skipBind){
    //SetActive returns immediately if it's already been called
    if (!skipBind)
        if(!SetActive()) return;

    glDrawElements(_typeInternal, _rangeCount, _formatInternal, VOID_PTR_CAST(_firstElement * (_largeIndices ? sizeof(GLuint) : sizeof(GLushort))));
    GL_API::registerDrawCall();
}

///This draw method needs an Index Buffer!
void glVertexArray::Draw(bool skipBind, const U8 LODindex){
    if (!skipBind)
        if(!SetActive()) return;

    U32 lod = std::min((U32)LODindex, (U32)_indiceLimits.size() - 1);
    GLsizei count = (GLsizei)(_largeIndices ? _hardwareIndicesL.size() : _hardwareIndicesS.size());

    assert(_indiceLimits[lod].y > _indiceLimits[lod].x && _indiceLimits[lod].y < (GLuint)count);

    glDrawRangeElements(_typeInternal, _indiceLimits[lod].x, _indiceLimits[lod].y, count, _formatInternal, BUFFER_OFFSET(0));
    GL_API::registerDrawCall();
 }

/// If we do not have a VAO, we use vertex arrays as fail safes
bool glVertexArray::SetActive(){
    //if the shader is not ready, do not draw anything
    if(!_currentShader->isBound() || GFXDevice::getActiveProgram()->getId() != _currentShader->getId())
        return false;

    // Make sure we have valid data
    if (!_created) CreateInternal();

    // Make sure we do not have a refresh request queued up
    if(_refreshQueued)
        if(!Refresh())
            return false;

    //Choose optimal VAO/VB combo
    setDepthPass(GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE));

    if (GL_API::setActiveVAO(_depthPass && _optimizeForDepth ? _DepthVAOid : _VAOid)){
        GL_API::togglePrimitiveRestart(_primitiveRestartEnabled, !_largeIndices);
    }

    //Send our transformation matrices (projection, world, view, inv model view, etc)
    currentShader()->uploadNodeMatrices();

    return true;
}

//Bind vertex attributes (only vertex, normal and animation data is used for the depth-only vb)
void glVertexArray::Upload_VB_Attributes(){
    glEnableVertexAttribArray(Divide::VERTEX_POSITION_LOCATION);
    glVertexAttribPointer(Divide::VERTEX_POSITION_LOCATION,
                                  3,
                                  GL_FLOAT, GL_FALSE,
                                  sizeof(vec3<GLfloat>),
                                  BUFFER_OFFSET(_VBoffsetPosition));
    if(!_dataColor.empty()) {
        glEnableVertexAttribArray(Divide::VERTEX_COLOR_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_COLOR_LOCATION,
                                      3,
                                      GL_UNSIGNED_BYTE, GL_FALSE,
                                      sizeof(vec3<GLubyte>),
                                      BUFFER_OFFSET(_VBoffsetColor));
    }
    if(!_dataNormal.empty()) {
        glEnableVertexAttribArray(Divide::VERTEX_NORMAL_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_NORMAL_LOCATION,
                                      3,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBoffsetNormal));
    }

    if(!_dataTexcoord.empty()) {
        glEnableVertexAttribArray(Divide::VERTEX_TEXCOORD_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_TEXCOORD_LOCATION,
                                      2,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec2<GLfloat>),
                                      BUFFER_OFFSET(_VBoffsetTexcoord));
    }

    if(!_dataTangent.empty()){
        glEnableVertexAttribArray(Divide::VERTEX_TANGENT_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_TANGENT_LOCATION,
                                      3,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBoffsetTangent));
    }

    if(!_dataBiTangent.empty()){
        glEnableVertexAttribArray(Divide::VERTEX_BITANGENT_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_BITANGENT_LOCATION,
                                      3,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBoffsetBiTangent));
    }

    if(_animationData){
        //Bone weights
        glEnableVertexAttribArray(Divide::VERTEX_BONE_WEIGHT_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_BONE_WEIGHT_LOCATION,
                                      4,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec4<GLfloat>),
                                      BUFFER_OFFSET(_VBoffsetBoneWeights));

        // Bone indices
        glEnableVertexAttribArray(Divide::VERTEX_BONE_INDICE_LOCATION);
        glVertexAttribIPointer(Divide::VERTEX_BONE_INDICE_LOCATION,
                                       4,
                                       GL_UNSIGNED_BYTE,
                                       sizeof(vec4<GLubyte>),
                                       BUFFER_OFFSET(_VBoffsetBoneIndices));
    }
}

void glVertexArray::Upload_VB_Depth_Attributes(){
    glEnableVertexAttribArray(Divide::VERTEX_POSITION_LOCATION);
    glVertexAttribPointer(Divide::VERTEX_POSITION_LOCATION,
                                  3,
                                  GL_FLOAT, GL_FALSE,
                                  sizeof(vec3<GLfloat>),
                                  BUFFER_OFFSET(_VBoffsetPosition));
    if(!_dataNormal.empty()) {
        glEnableVertexAttribArray(Divide::VERTEX_NORMAL_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_NORMAL_LOCATION,
                                      3,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBoffsetNormal));
    }

    if(_animationData){
        //Bone weights
        glEnableVertexAttribArray(Divide::VERTEX_BONE_WEIGHT_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_BONE_WEIGHT_LOCATION,
                                      4,
                                      GL_FLOAT, GL_FALSE,
                                      sizeof(vec4<GLfloat>),
                                      BUFFER_OFFSET(_VBoffsetBoneWeightsDEPTH));

        // Bone indices
        glEnableVertexAttribArray(Divide::VERTEX_BONE_INDICE_LOCATION);
        glVertexAttribIPointer(Divide::VERTEX_BONE_INDICE_LOCATION,
                                       4,
                                       GL_UNSIGNED_BYTE,
                                       sizeof(vec4<GLubyte>),
                                       BUFFER_OFFSET(_VBoffsetBoneIndicesDEPTH));
    }
}

inline bool degenerateTriangleMatch(const vec3<U32>& triangle){
    return (triangle.x != triangle.y && triangle.x != triangle.z && triangle.y != triangle.z);
}

//Create a list of triangles from the vertices + indices lists based on primitive type
bool glVertexArray::computeTriangleList(){
    //We can't have a VB without vertex positions
    assert(!_dataPosition.empty());

    if(!_computeTriangles) return true;
    if(!_dataTriangles.empty()) return true;
    if(_hardwareIndicesL.empty() && _hardwareIndicesS.empty()) return false;

    size_t indiceCount = _largeIndices ? _hardwareIndicesL.size() : _hardwareIndicesS.size();

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
void glVertexArray::checkStatus(){
}