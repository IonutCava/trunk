#include "glResources.h"
#include "resource.h"
#include "glVertexBufferObject.h"


glVertexBufferObject::glVertexBufferObject(){
	_VBOid = 0;
	_created = false;

}

void glVertexBufferObject::Destroy(){
	glDeleteBuffers(1, &_VBOid);
}


bool glVertexBufferObject::Create(){	
	return Create(GL_STATIC_DRAW);
}

bool glVertexBufferObject::Create(U32 usage)
{
	_VBOid = 0;

	ptrdiff_t	nSizePosition = 0;
	ptrdiff_t	nSizeNormal = 0;
	ptrdiff_t	nSizeTexcoord = 0;
	ptrdiff_t	nSizeTangent = 0;
	ptrdiff_t	nSizeBiTangent = 0;


	if(!_dataPosition.empty()) {
		nSizePosition = _dataPosition.size()*sizeof(vec3);
	}
	else {
		ERROR_F("No position data !\n");
		return false;
	}

	

	if(!_dataNormal.empty()) {
		nSizeNormal	= _dataNormal.size()*sizeof(vec3);
	}

	if(!_dataTexcoord.empty()) {
		nSizeTexcoord = _dataTexcoord.size()*sizeof(vec2);
	}

	if(!_dataTangent.empty()) {
		nSizeTangent = _dataTangent.size()*sizeof(vec3);
	}

	if(!_dataBiTangent.empty()) {
		nSizeBiTangent = _dataBiTangent.size()*sizeof(vec3);
	}


	_VBOoffsetPosition	= 0;
	_VBOoffsetNormal	= _VBOoffsetPosition + nSizePosition;
	_VBOoffsetTexcoord	= _VBOoffsetNormal + nSizeNormal;
	_VBOoffsetTangent	= _VBOoffsetTexcoord + nSizeTexcoord;
	_VBOoffsetBiTangent	= _VBOoffsetTangent + nSizeTangent;

	glGenBuffers(1, &_VBOid);
	if(_VBOid == 0) {
		ERROR_FN( "Init VBO failed!");
		_created = false;
	}
	else {
		glBindBuffer(GL_ARRAY_BUFFER, _VBOid);
		glBufferData(GL_ARRAY_BUFFER, nSizePosition+nSizeNormal+nSizeTexcoord+nSizeTangent+nSizeBiTangent, 0, usage);

		glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetPosition,	nSizePosition,	(const GLvoid*)(&_dataPosition[0]));

		if(_dataNormal.size())
			glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetNormal,	nSizeNormal,	(const GLvoid*)(&_dataNormal[0]));

		if(_dataTexcoord.size())
			glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetTexcoord,	nSizeTexcoord,	(const GLvoid*)(&_dataTexcoord[0]));

		if(_dataTangent.size())
			glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetTangent,	nSizeTangent,	(const GLvoid*)(&_dataTangent[0]));

		if(_dataBiTangent.size())
			glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBiTangent,	nSizeBiTangent,	(const GLvoid*)(&_dataBiTangent[0]));

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	_created = true;
	return _created;
}


void glVertexBufferObject::Enable(){
	if(!_created) Create();
	if(_VBOid)	    Enable_VBO();		
	else			Enable_VA();		
}

void glVertexBufferObject::Disable()
{
	if(_VBOid)	Disable_VBO();		
	else		Disable_VA();		
}

void glVertexBufferObject::Enable_VA()
{
	U16 slot = 0;
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &(_dataPosition[0].x));

	if(!_dataNormal.empty()) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, &(_dataNormal[0].x));
	}

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, &(_dataTexcoord[0].s));
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, 0, &(_dataTangent[0].x));
	}

	if(!_dataBiTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, 0, &(_dataBiTangent[0].x));
	}
}

void glVertexBufferObject::Enable_VBO()
{
	U16 slot = 0;
	glBindBuffer(GL_ARRAY_BUFFER, _VBOid);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, (const GLvoid*)_VBOoffsetPosition);

	if(!_dataNormal.empty()) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, (const GLvoid*)_VBOoffsetNormal);
	}

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, (const GLvoid*)_VBOoffsetTexcoord);
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, 0, (const GLvoid*)_VBOoffsetTangent);
	}

	if(!_dataBiTangent.empty()){
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, 0, (const GLvoid*)_VBOoffsetBiTangent);
	}

}

void glVertexBufferObject::Disable_VA()
{
	U16 slot = 0;

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataBiTangent.empty()){
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataNormal.empty()) {
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}

void glVertexBufferObject::Disable_VBO()
{
	U16 slot = 0;

	if(!_dataTexcoord.empty()) {
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataTangent.empty()) {
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataBiTangent.empty()){
		glClientActiveTexture(GL_TEXTURE0 + slot++);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if(!_dataNormal.empty()) {
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}



