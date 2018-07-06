class Bone
{
public:
	Bone(int id, F32* absMatrix, F32* relMatrix, F32 length) :
	  _id(id),  
	  _absMatrix(absMatrix),
	  _relMatrix(relMatrix),
	  _length(length)
	  {}
 int _id;
 F32 _absMatrix[16], _relMatrix[16];
 F32 _length;
 Bone _parent, _child, _brother;
};