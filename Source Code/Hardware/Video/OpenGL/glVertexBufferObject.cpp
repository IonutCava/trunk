#include "glResources.h"
#include "resource.h"
#include "glVertexBufferObject.h"

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
	/// Internally call the apropriate construction method
	if(staticDraw){
		return Create((U32)GL_STATIC_DRAW);
	}
	return Create((U32)GL_DYNAMIC_DRAW);
}

/// Update internal data based on construction method (dynamic/static)
bool glVertexBufferObject::Refresh(){
	U32 usage = GL_STATIC_DRAW;
	if(!_staticDraw){
		usage = GL_DYNAMIC_DRAW;
	}

	ptrdiff_t	nSizePosition = 0;
	ptrdiff_t	nSizeNormal = 0;
	ptrdiff_t	nSizeTexcoord = 0;
	ptrdiff_t	nSizeTangent = 0;
	ptrdiff_t	nSizeBiTangent = 0;
	ptrdiff_t	nSizeBoneIndices = 0;
	ptrdiff_t	nSizeBoneWeights = 0;

	if(!_dataPosition.empty()) {
		nSizePosition = _dataPosition.size()*sizeof(vec3<F32>);
	}
	else {
		ERROR_F("No position data !\n");
		return false;
	}

	if(!_dataNormal.empty()) {
		nSizeNormal	= _dataNormal.size()*sizeof(vec3<F32>);
	}

	if(!_dataTexcoord.empty()) {
		nSizeTexcoord = _dataTexcoord.size()*sizeof(vec2<F32>);
	}

	if(!_dataTangent.empty()) {
		nSizeTangent = _dataTangent.size()*sizeof(vec3<F32>);
	}

	if(!_dataBiTangent.empty()) {
		nSizeBiTangent = _dataBiTangent.size()*sizeof(vec3<F32>);
	}

	if(!_boneIndices.empty()){
		nSizeBoneIndices = _boneIndices.size()*sizeof(vec4<I16>);
	}

	if(!_boneWeights.empty()){
		nSizeBoneWeights = _boneWeights.size()*sizeof(vec4<F32>);
	}



	_VBOoffsetPosition	  = 0;
	_VBOoffsetNormal	  = _VBOoffsetPosition + nSizePosition;
	_VBOoffsetTexcoord	  = _VBOoffsetNormal + nSizeNormal;
	_VBOoffsetTangent	  = _VBOoffsetTexcoord + nSizeTexcoord;
	_VBOoffsetBiTangent	  = _VBOoffsetTangent + nSizeTangent;
	_VBOoffsetBoneIndices = _VBOoffsetBiTangent + nSizeBiTangent;
	_VBOoffsetBoneWeights = _VBOoffsetBoneIndices + nSizeBoneIndices;

	glBindBuffer(GL_ARRAY_BUFFER, _VBOid);
	glBufferData(GL_ARRAY_BUFFER, nSizePosition+
								  nSizeNormal+
								  nSizeTexcoord+
								  nSizeTangent+
								  nSizeBiTangent+
								  nSizeBoneIndices+
								  nSizeBoneWeights,
								  0, usage);
	if(_positionDirty)
	    glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetPosition,	nSizePosition,	    (const GLvoid*)(&_dataPosition[0]));

	if(_dataNormal.size() && _normalDirty)
		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetNormal,	    nSizeNormal,	    (const GLvoid*)(&_dataNormal[0]));

	if(_dataTexcoord.size() && _texcoordDirty)
		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetTexcoord,	nSizeTexcoord,   	(const GLvoid*)(&_dataTexcoord[0]));

	if(_dataTangent.size() && _tangentDirty)
		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetTangent,	    nSizeTangent,	    (const GLvoid*)(&_dataTangent[0]));

	if(_dataBiTangent.size() && _bitangentDirty)
		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBiTangent,	nSizeBiTangent,	    (const GLvoid*)(&_dataBiTangent[0]));

	if(_boneIndices.size() && _indicesDirty)
		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneIndices,	nSizeBoneIndices,	(const GLvoid*)(&_boneIndices[0]));

	if(_boneWeights.size() && _weightsDirty)
		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneWeights,	nSizeBoneWeights,	(const GLvoid*)(&_boneWeights[0]));

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
bool glVertexBufferObject::Create(U32 usage) {
	_VBOid = 0;
	/// Generate a new buffer
	glGenBuffers(1, &_VBOid);
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

/// If we do not have a VBO object, we use vertex arrays as fail safes
void glVertexBufferObject::Enable(){
	if(!_created) Create();
	if(_VBOid)	    Enable_VBO();		
	else			Enable_VA();		
}
/// If we do not have a VBO object, we use vertex arrays as fail safes
void glVertexBufferObject::Disable(){
	if(_VBOid)	Disable_VBO();		
	else		Disable_VA();		
}

/// If we do not have a VBO object, we use vertex arrays as fail safes
void glVertexBufferObject::Enable_VA() {
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &(_dataPosition[0].x));

	if(!_dataNormal.empty()) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, &(_dataNormal[0].x));
	}

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, &(_dataTexcoord[0].s));
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, 0, &(_dataTangent[0].x));
	}

	if(!_dataBiTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, 0, &(_dataBiTangent[0].x));
	}

	if(!_boneIndices.empty()){
		glClientActiveTexture(GL_TEXTURE3);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(4, GL_SHORT, 0, &(_boneIndices[0].x));
	}

	if(!_boneWeights.empty()){
		glClientActiveTexture(GL_TEXTURE4);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(4, GL_FLOAT, 0, &(_boneWeights[0].x));
	}
}

/// If we do not have a VBO object, we use vertex arrays as fail safes
void glVertexBufferObject::Disable_VA() {

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE1);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataBiTangent.empty()){
		glClientActiveTexture(GL_TEXTURE2);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_boneIndices.empty()){
		glClientActiveTexture(GL_TEXTURE3);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_boneWeights.empty()){
		glClientActiveTexture(GL_TEXTURE4);
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
	glVertexPointer(3, GL_FLOAT, 0, (const GLvoid*)_VBOoffsetPosition);

	if(!_dataNormal.empty()) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, (const GLvoid*)_VBOoffsetNormal);
	}

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, (const GLvoid*)_VBOoffsetTexcoord);
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, 0, (const GLvoid*)_VBOoffsetTangent);
	}

	if(!_dataBiTangent.empty()){
		glClientActiveTexture(GL_TEXTURE2);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, 0, (const GLvoid*)_VBOoffsetBiTangent);
	}

	if(!_boneIndices.empty()){
		glClientActiveTexture(GL_TEXTURE3);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(4, GL_SHORT, 0, (const GLvoid*)_VBOoffsetBoneIndices);
	}

	if(!_boneWeights.empty()){
		glClientActiveTexture(GL_TEXTURE4);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(4, GL_FLOAT, 0, (const GLvoid*)_VBOoffsetBoneWeights);
	}
}

void glVertexBufferObject::Disable_VBO(){

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE1);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataBiTangent.empty()){
		glClientActiveTexture(GL_TEXTURE2);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_boneIndices.empty()){
		glClientActiveTexture(GL_TEXTURE3);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_boneWeights.empty()){
		glClientActiveTexture(GL_TEXTURE4);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataNormal.empty()) {
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}



