#include "Headers/AnimationUtils.h"
#include "Hardware/Video/Headers/GFXDevice.h"

namespace AnimUtils {
	/// there is some type of alignment issue with my mat4 and the aimatrix4x4 class, so the copy must be done manually
	void TransformMatrix(mat4<F32>& out,const aiMatrix4x4& in){
		bool colMaj = (GFX_DEVICE.getApi() != Direct3D);
		out.element(0,0,colMaj)=in.a1;
		out.element(0,1,colMaj)=in.b1;
		out.element(0,2,colMaj)=in.c1;
		out.element(0,3,colMaj)=in.d1;

		out.element(1,0,colMaj)=in.a2;
		out.element(1,1,colMaj)=in.b2;
		out.element(1,2,colMaj)=in.c2;
		out.element(1,3,colMaj)=in.d2;

		out.element(2,0,colMaj)=in.a3;
		out.element(2,1,colMaj)=in.b3;
		out.element(2,2,colMaj)=in.c3;
		out.element(2,3,colMaj)=in.d3;

		out.element(3,0,colMaj)=in.a4;
		out.element(3,1,colMaj)=in.b4;
		out.element(3,2,colMaj)=in.c4;
		out.element(3,3,colMaj)=in.d4;
	}

	void TransformMatrix(aiMatrix4x4& out,const mat4<F32>& in){
		bool colMaj = (GFX_DEVICE.getApi() != Direct3D);
		out.a1 = in.element(0,0,colMaj);
		out.b1 = in.element(0,1,colMaj);
		out.c1 = in.element(0,2,colMaj);
		out.d1 = in.element(0,3,colMaj);

		out.a2 = in.element(1,0,colMaj);
		out.b2 = in.element(1,1,colMaj);
		out.c2 = in.element(1,2,colMaj);
		out.d2 = in.element(1,3,colMaj);

		out.a3 = in.element(2,0,colMaj);
		out.b3 = in.element(2,1,colMaj);
		out.c3 = in.element(2,2,colMaj);
		out.d3 = in.element(2,3,colMaj);

		out.a4 = in.element(3,0,colMaj);
		out.b4 = in.element(3,1,colMaj);
		out.c4 = in.element(3,2,colMaj);
		out.d4 = in.element(3,3,colMaj);
	}
}