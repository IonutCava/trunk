#ifndef CAMERA_H
#define CAMERA_H

#include "resource.h"
#include "Utility/Headers/Singleton.h"
#include "Utility/Headers/MathClasses.h"
class vec3;
SINGLETON_BEGIN( Camera)

public:	enum TYPE {FREE, DRIVEN};
private:
	vec3	vEye;
	vec3	vCenter;
	vec3	vViewDir;
	vec3	vLeftDir;	
	vec3	vUp;

	F32	fAngleX;
	F32	fAngleY;	
	TYPE	eType;


	vec3	tSaveVectors[5];
	F32	tSaveFloats[2];
	bool	bSaved;
	F32   terHeightMin;

public:
	inline TYPE  getType()				const	{return eType;}
	inline const vec3& getEye()			const	{return vEye;}
	inline const vec3& getCenter()		const	{return vCenter;}
	inline const vec3& getViewDir()		const	{return vViewDir;}
	inline F32 getAngleX()			const	{return fAngleX;}
	inline F32 getAngleY()			const	{return fAngleY;}


protected:
	Camera();

public:
	void SaveCamera();
	void RestoreCamera();

	void Refresh();

	void setType(TYPE t)				{ eType = t;		Refresh(); }
	void setEye(vec3 vPos)				{ vEye = vPos;		Refresh(); }
	void setCenter(vec3 vPos)			{ vCenter = vPos;	Refresh(); }
	void setAngleX(F32 Angle)			{ fAngleX = Angle;	Refresh(); }
	void setAngleY(F32 Angle)			{ fAngleY = Angle;	Refresh(); }
	void setAngle(F32 AngleX, F32 AngleY)	{
		fAngleX = AngleX;
		fAngleY = AngleY;
		Refresh();
	}

	void Translate(vec3 vector)	{ vEye += vector;	Refresh(); }
	void RotateX(F32 AngleX)		{
		fAngleX += AngleX;
				if(fAngleX<0.0f)	fAngleX += M_PI*2;
		else	if(fAngleX>M_PI*2)	fAngleX -= M_PI*2;
		Refresh();
	}
	void RotateY(F32 AngleY)		{
		fAngleY += AngleY;
		if(fAngleY<0.0f || fAngleY>M_PI)	fAngleY -= AngleY;
		Refresh();
	}
	void Rotate(F32 AngleX, F32 AngleY)		{
		fAngleX += AngleX;
		fAngleY += AngleY;
				if(fAngleX<0.0f)		fAngleX += M_PI*2;
		else	if(fAngleX>M_PI*2)		fAngleX -= M_PI*2;
		if(fAngleY<0 || fAngleY>M_PI)	fAngleY -= AngleY;
		Refresh();
	}

	void PlayerMoveForward(F32 factor);	
	void PlayerMoveStrafe(F32 factor);
	void TranslateForward(F32 factor);
	void TranslateStrafe(F32 factor);	
	void MoveAnaglyph(F32 factor);

	void RenderLookAt(bool inverty=false, F32 planey=0.0f);

	void RenderLookAtToCubeMap(const vec3& eye, U32 nFace);
	
	SINGLETON_END()
#endif

