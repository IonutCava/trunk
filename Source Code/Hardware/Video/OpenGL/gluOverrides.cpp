#include "Headers/glResources.h" 
#include "Core/Math/Headers/MathClasses.h"
#include "Rendering/Headers/Frustum.h"

namespace Divide {
	namespace GL {
/*----------- GLU overrides ------*/
static void _makeIdentityd(GLdouble m[16]){
    m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
    m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
    m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
    m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}

static void _makeIdentityf(GLfloat m[16]){
    m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
    m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
    m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
    m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}

mat4<GLfloat> _ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar){
	glOrtho(left,right,bottom,top,zNear,zFar);	

	mat4<GLfloat> Matrix;
	Matrix.element(0,0) = 2 / (right - left);
	Matrix.element(1,1) = 2 / (top - bottom);
	Matrix.element(2,2) = - 2 / (zFar - zNear);
	Matrix.element(3,0) = - (right + left) / (right - left);
	Matrix.element(3,1) = - (top + bottom) / (top - bottom);
	Matrix.element(3,2) = - (zFar + zNear) / (zFar - zNear);
	
	return Matrix;

}

mat4<GLfloat> _perspective(GLfloat fovy, 
			  		       GLfloat aspect,
					       GLfloat zNear,
					       GLfloat zFar){
    GLfloat m[4][4];
    GLdouble sine, cotangent, deltaZ;
    GLdouble radians = RADIANS(fovy/2);

    deltaZ = zFar - zNear;
    sine = sin(radians);
    if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
		return mat4<GLfloat>();
    }

    cotangent = cos(radians) / sine;

    _makeIdentityf(&m[0][0]);

    m[0][0] = cotangent / aspect;
    m[1][1] = cotangent;
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1;
    m[3][2] = -2 * zNear * zFar / deltaZ;
    m[3][3] = 0;

    glMultMatrixf(&m[0][0]);
	mat4<GLfloat> Matrix(&m[0][0]);
	return Matrix;
}

void _CrossProd(GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2, GLfloat res[3]){
	res[0] = y1*z2 - y2*z1;
	res[1] = x2*z1 - x1*z2;
	res[2] = x1*y2 - x2*y1;
}


///Code by Fadi Chehimi from http://www.khronos.org/message_boards/viewtopic.php?f=4&t=502
mat4<GLfloat> _LookAt(GLfloat eyeX, 
				      GLfloat eyeY,
		 			  GLfloat eyeZ,
		 			  GLfloat lookAtX,
					  GLfloat lookAtY,
					  GLfloat lookAtZ,
					  GLfloat upX,
					  GLfloat upY,
					  GLfloat upZ){

	GLfloat forward[3];
	GLfloat side[3];
	GLfloat up[3];

	// calculating the viewing vector
	forward[0] = lookAtX - eyeX;
	forward[1] = lookAtY - eyeY;
	forward[2] = lookAtZ - eyeZ;

	GLfloat forwardMag = square_root_tpl<GLfloat>(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
	GLfloat upMag = square_root_tpl<GLfloat>(upX*upX + upY*upY + upZ*upZ);

	// normalizing the viewing vector
	if( forwardMag != 0.0){
		forward[0] /= forwardMag;
		forward[1] /= forwardMag;
		forward[2] /= forwardMag;
	}

	// normalising the up vector. no need for this here if you have your
	// up vector already normalised, which is mostly the case.
	if( upMag != 0.0 ){
		upX /= upMag;
		upY /= upMag;
		upZ /= upMag;
	}

	_CrossProd(forward[0], forward[1], forward[2], upX, upY, upZ, side);
	GLfloat sideMag = square_root_tpl<GLfloat>(side[0]*side[0] + side[1]*side[1] + side[2]*side[2]);

	if( sideMag != 0.0 ){
		side[0] /= sideMag;
		side[1] /= sideMag;
		side[2] /= sideMag;
	}

	_CrossProd(side[0], side[1], side[2], forward[0], forward[1], forward[2], up);
	
	GLfloat m[] =
	{
		side[0], up[0], -forward[0], 0,
		side[1], up[1], -forward[1], 0,
		side[2], up[2], -forward[2], 0,
		0,       0,     0,           1
	};

	
	glMultMatrixf(m);
	glTranslatef (-eyeX, -eyeY, -eyeZ);
	
	mat4<GLfloat> Matrix(m);
	Matrix.translate(-eyeX, -eyeY, -eyeZ);
	return Matrix;
}
	}//namespace GL
}// namespace Divide