#include "glResources.h"
#include "resource.h"
#include "glVertexBufferObject.h"
#include "Hardware/Video/GFXDevice.h"

/// Default destructor
glVertexBufferObject::glVertexBufferObject() : VertexBufferObject() {
	_created = false;
}

/// Delete buffer
void glVertexBufferObject::Destroy(){
	glDeleteBuffers(1, &_VBOid);
}

/// Create a dynamic or static VBO
bool glVertexBufferObject::Create(bool staticDraw){	
	_staticDraw = staticDraw;
	return CreateInternal();
}

/// Update internal data based on construction method (dynamic/static)
bool glVertexBufferObject::Refresh(){
	U32 usage = GL_STATIC_DRAW;

	if(!_staticDraw){
		(GFX_DEVICE.getApi() == OpenGLES) ?  usage = GL_STREAM_DRAW :  usage = GL_DYNAMIC_DRAW;
	}

	if(!_hardwareIndices.empty()){
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBOid);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, _hardwareIndices.size() * 2, &_hardwareIndices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	ptrdiff_t	nSizePosition = 0;
	ptrdiff_t	nSizeNormal = 0;
	ptrdiff_t	nSizeTexcoord = 0;
	ptrdiff_t	nSizeTangent = 0;
	ptrdiff_t	nSizeBiTangent = 0;
	ptrdiff_t	nSizeBoneWeights = 0;
	ptrdiff_t	nSizeBoneIndices = 0;

	if(!_dataPosition.empty()) {
		nSizePosition = _dataPosition.size()*sizeof(vec3<F32>);
	}
	else {
		ERROR_F("No position data !\n");
		return false;
	}

	nSizeNormal	= _dataNormal.size()*sizeof(vec3<F32>);
	nSizeTexcoord = _dataTexcoord.size()*sizeof(vec2<F32>);
	nSizeTangent = _dataTangent.size()*sizeof(vec3<F32>);
	nSizeBiTangent = _dataBiTangent.size()*sizeof(vec3<F32>);
	/// Bone indices and weights are always packed togheter to prevent invalid animations
	nSizeBoneWeights = _boneWeights.size()*sizeof(vec4<F32>);
	nSizeBoneIndices = _boneIndices.size()*sizeof(vec4<U8>);
	
	_VBOoffsetPosition  = 0;
	ptrdiff_t previousOffset = _VBOoffsetPosition;
	ptrdiff_t previousCount = nSizePosition;
	if(nSizeNormal > 0){
		_VBOoffsetNormal = previousOffset + previousCount;
		previousOffset = _VBOoffsetNormal;
		previousCount = nSizeNormal;
	}

	if(nSizeTexcoord > 0){
		_VBOoffsetTexcoord	= previousOffset + previousCount;
		previousOffset = _VBOoffsetTexcoord;
		previousCount = nSizeTexcoord;
	}

	if(nSizeTangent > 0){
		_VBOoffsetTangent	= previousOffset + previousCount;
		previousOffset = _VBOoffsetTangent;
		previousCount = nSizeTangent;
	}

	if(nSizeBiTangent > 0){
		_VBOoffsetBiTangent	  = previousOffset + previousCount;
		previousOffset = _VBOoffsetBiTangent;
		previousCount = nSizeBiTangent;
	}

	if(nSizeBoneWeights > 0 && nSizeBoneIndices > 0){
		_VBOoffsetBoneIndices = previousOffset + previousCount;
		previousOffset = _VBOoffsetBoneIndices;
		previousCount = nSizeBoneIndices;
		_VBOoffsetBoneWeights = previousOffset + previousCount;
	}


	glBindBuffer(GL_ARRAY_BUFFER, _VBOid);
	glBufferData(GL_ARRAY_BUFFER, nSizePosition+
								  nSizeNormal+
								  nSizeTexcoord+
								  nSizeTangent+
								  nSizeBiTangent+
								  nSizeBoneIndices+
								  nSizeBoneWeights,
								  0, usage);
	if(_positionDirty){
	    glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetPosition,	nSizePosition,	    
		                                 (const GLvoid*)(&_dataPosition[0].x));
	}

	if(_dataNormal.size() && _normalDirty){
		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetNormal,	    nSizeNormal,	   
		                                (const GLvoid*)(&_dataNormal[0].x));
	}

	if(_dataTexcoord.size() && _texcoordDirty){
		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetTexcoord,	nSizeTexcoord,   	
		                                (const GLvoid*)(&_dataTexcoord[0].s));
	}

	if(_dataTangent.size() && _tangentDirty){
		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetTangent,	    nSizeTangent,	    
		                                 (const GLvoid*)(&_dataTangent[0].x));
	}

	if(_dataBiTangent.size() && _bitangentDirty){
		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBiTangent,	nSizeBiTangent,	    
		                                (const GLvoid*)(&_dataBiTangent[0].x));
	}

	if(_boneWeights.size() && _boneIndices.size()) {
		if(_indicesDirty){
			glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneIndices,	nSizeBoneIndices,	
								             (const GLvoid*)(&_boneIndices[0].x));
		}
		if(_weightsDirty){
			glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneWeights,	nSizeBoneWeights,	
							                 (const GLvoid*)(&_boneWeights[0].x));
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/// Clear all update flags
	_positionDirty  = false;
	_normalDirty    = false;
	_texcoordDirty  = false;
	_tangentDirty   = false;
	_bitangentDirty = false;
	_indicesDirty   = false;
	_weightsDirty   = false;

	return true;
}


/// This method creates the initial VBO
/// The only diferrence between Create and Refresh is the generation of a new buffer
bool glVertexBufferObject::CreateInternal() {
	_VBOid = 0;
	/// Generate a new buffer
	glGenBuffers(1, &_VBOid);
	/// Generate an "IndexBufferObject"
	if(!_hardwareIndices.empty()){
		glGenBuffers(1, &_IBOid);
	}
	/// Enforce all update flags. Kinda useless, but it doesn't hurt
	_positionDirty  = true;
	_normalDirty    = true;
	_texcoordDirty  = true;
	_tangentDirty   = true;
	_bitangentDirty = true;
	_indicesDirty   = true;
	_weightsDirty   = true;
	/// Validate buffer creation
	if(!_VBOid) {
		ERROR_FN( "Init VBO failed!");
		_created = false;
	}else {
		/// calling refresh updates all stored information and sends it to the GPU
		_created = Refresh();
	}
	return _created;
}

/// Inform the VBO what shader we use to render the object so that the data is uploaded correctly
void glVertexBufferObject::setShaderProgram(ShaderProgram* const shaderProgram) {
	_currentShader = shaderProgram;
	if(_currentShader != NULL){
		bool shaderState = _currentShader->isBound();
		if(!shaderState){
			_currentShader->bind();
		}
		/// Inform the shader about the data upload method
		_currentShader->Uniform("useVBOVertexData",true);
		/// Update data locations
		_positionDataLocation   = _currentShader->getAttributeLocation("inVertexData");
		_normalDataLocation     = _currentShader->getAttributeLocation("inNormalData");
		_texCoordDataLocation   = _currentShader->getAttributeLocation("inTexCoordData");
		_tangentDataLocation    = _currentShader->getAttributeLocation("inTangentData");
		_biTangentDataLocation  = _currentShader->getAttributeLocation("inBiTangentData");
		_boneIndiceDataLocation = _currentShader->getAttributeLocation("inBoneIndiceData");
		_boneWeightDataLocation = _currentShader->getAttributeLocation("inBoneWeightData");
		if(!shaderState){
			_currentShader->unbind();
		}
	}
}

/// If we do not have a VBO object, we use vertex arrays as fail safes
void glVertexBufferObject::Enable(){
	if(!_created) CreateInternal();
	if(_VBOid){
		if(_IBOid){
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBOid);
		}
		/// If we do not have a shader assigned, use old style VBO
		if(_currentShader == NULL){
			Enable_VBO();		
		}else{ /// Else, use attrib pointers
			Enable_Shader_VBO();
		}
	}else{
		Enable_VA();		
	}
}
/// If we do not have a VBO object, we use vertex arrays as fail safes
void glVertexBufferObject::Disable(){
	if(_VBOid){
		/// If we do not have a shader assigned, use old style VBO
		if(_currentShader == NULL){
			Disable_VBO();		
		}else{
			Disable_Shader_VBO();
		}
		if(_IBOid){
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}else{
		Disable_VA();		
	}
}

/// If we do not have a VBO object, we use vertex arrays as fail safes
void glVertexBufferObject::Enable_VA() {

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(vec3<F32>), &(_dataPosition[0].x));

	if(!_dataNormal.empty()) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(vec3<F32>), &(_dataNormal[0].x));
	}

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(vec2<F32>), &(_dataTexcoord[0].s));
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(vec3<F32>), &(_dataTangent[0].x));
	}

	if(!_dataBiTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(vec3<F32>), &(_dataBiTangent[0].x));
	}

	if(!_boneWeights.empty() && !_boneIndices.empty()){
		glClientActiveTexture(GL_TEXTURE3);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(4, GL_BYTE,sizeof(vec4<U8>), &(_boneIndices[0].x));

		glClientActiveTexture(GL_TEXTURE4);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(4, GL_FLOAT, sizeof(vec4<F32>), &(_boneWeights[0].x));

	}
}

/// If we do not have a VBO object, we use vertex arrays as fail safes
void glVertexBufferObject::Disable_VA() {

	if(!_boneWeights.empty() && !_boneIndices.empty()){
		glClientActiveTexture(GL_TEXTURE4);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glClientActiveTexture(GL_TEXTURE3);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataBiTangent.empty()){
		glClientActiveTexture(GL_TEXTURE2);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE1);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	
	if(!_dataNormal.empty()) {
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}


void glVertexBufferObject::Enable_VBO() {

	glBindBuffer(GL_ARRAY_BUFFER, _VBOid);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(vec3<F32>), (const GLvoid*)_VBOoffsetPosition);

	if(!_dataNormal.empty()) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(vec3<F32>), (const GLvoid*)_VBOoffsetNormal);
	}

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(vec2<F32>), (const GLvoid*)_VBOoffsetTexcoord);
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(vec3<F32>), (const GLvoid*)_VBOoffsetTangent);
	}

	if(!_dataBiTangent.empty()){
		glClientActiveTexture(GL_TEXTURE2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(vec3<F32>), (const GLvoid*)_VBOoffsetBiTangent);
	}

	if(!_boneWeights.empty() && !_boneIndices.empty()){
		glClientActiveTexture(GL_TEXTURE3);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(4, GL_BYTE, sizeof(vec4<U8>),  (const GLvoid*)_VBOoffsetBoneIndices);

		glClientActiveTexture(GL_TEXTURE4);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(4, GL_FLOAT, sizeof(vec4<F32>), (const GLvoid*)_VBOoffsetBoneWeights);
	}
}

void glVertexBufferObject::Disable_VBO(){

	if(!_boneIndices.empty() && !_boneWeights.empty()){
		glClientActiveTexture(GL_TEXTURE4);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glClientActiveTexture(GL_TEXTURE3);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataBiTangent.empty()){
		glClientActiveTexture(GL_TEXTURE2);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE1);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataNormal.empty()) {
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}



void glVertexBufferObject::Enable_Shader_VBO(){
	assert(_currentShader != NULL); ///< Should never fail

	_currentShader->Uniform("emptyNormal",_dataNormal.empty());
	_currentShader->Uniform("emptyTexCoord",_dataTexcoord.empty());
	_currentShader->Uniform("emptyTangent",_dataTangent.empty());
	_currentShader->Uniform("emptyBiTangent",_dataBiTangent.empty());
	_currentShader->Uniform("emptyBones",(_boneIndices.empty() || _boneWeights.empty()));

	glBindBuffer(GL_ARRAY_BUFFER, _VBOid);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(vec3<F32>), (const GLvoid*)_VBOoffsetPosition);

	//glEnableVertexAttribArray(_positionDataLocation);
    //glVertexAttribPointer(_positionDataLocation, 3, GL_FLOAT, GL_FALSE, sizeof(vec3<F32>),(const GLvoid*)_VBOoffsetPosition);

	if(!_dataNormal.empty()) {
		glEnableVertexAttribArray(_normalDataLocation);
		glVertexAttribPointer(_normalDataLocation, 3, GL_FLOAT, GL_FALSE, sizeof(vec3<F32>), (const GLvoid*)_VBOoffsetNormal);
	}

	if(!_dataTexcoord.empty()) {
	    glEnableVertexAttribArray(_texCoordDataLocation);
		glVertexAttribPointer(_texCoordDataLocation, 2, GL_FLOAT, GL_FALSE, sizeof(vec2<F32>), (const GLvoid*)_VBOoffsetTexcoord);
	}

	if(!_dataTangent.empty()){
		glEnableVertexAttribArray(_tangentDataLocation);
		glVertexAttribPointer(_tangentDataLocation, 3, GL_FLOAT, GL_FALSE, sizeof(vec3<F32>), (const GLvoid*)_VBOoffsetTangent);
	}

	if(!_dataBiTangent.empty()){
		glEnableVertexAttribArray(_biTangentDataLocation);
		glVertexAttribPointer(_biTangentDataLocation, 3, GL_FLOAT, GL_FALSE, sizeof(vec3<F32>), (const GLvoid*)_VBOoffsetBiTangent);
	}

	if(!_boneIndices.empty() && !_boneWeights.empty()){
		/// Bone indices
		glEnableVertexAttribArray(_boneIndiceDataLocation);
		glVertexAttribIPointer(_boneIndiceDataLocation, 4, GL_BYTE, sizeof(vec4<U8>), (const GLvoid*)_VBOoffsetBoneIndices);

		/// Bone weights
		glEnableVertexAttribArray(_boneWeightDataLocation);
		glVertexAttribPointer(_boneWeightDataLocation,  4, GL_FLOAT, GL_FALSE, sizeof(vec4<F32>), (const GLvoid*)_VBOoffsetBoneWeights);

	}

}

void glVertexBufferObject::Disable_Shader_VBO(){
	assert(_currentShader != NULL); ///< Should never fail

	if(!_boneIndices.empty() && _boneWeights.empty()){
		glDisableVertexAttribArray(_boneWeightDataLocation);
		glDisableVertexAttribArray(_boneIndiceDataLocation);
	}

	if(!_dataBiTangent.empty()){
		glDisableVertexAttribArray(_biTangentDataLocation);
	}

	if(!_dataTangent.empty()) {
		glDisableVertexAttribArray(_tangentDataLocation);
	}

	if(!_dataTexcoord.empty()) {
		glDisableVertexAttribArray(_texCoordDataLocation);
	}

	if(!_dataNormal.empty()) {
		glDisableVertexAttribArray(_normalDataLocation);
	}

	//glDisableVertexAttribArray(_positionDataLocation);
	glDisableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}