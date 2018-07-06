/*“Copyright 2009-2012 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CAMERA_H
#define _CAMERA_H

#include "resource.h"
#include "Core/Headers/BaseClasses.h"

class Camera : public Resource {

public:	enum TYPE {FREE_FLY, FIRST_PERSON, THIRD_PERSON, ORBIT,SCRIPTED};
protected:
	vec3<F32>	vEye;
	vec3<F32>	vCenter;
	vec3<F32>	vViewDir;
	vec3<F32>	vLeftDir;	
	vec3<F32>	vUp;

	F32		fAngleX;
	F32		fAngleY;	
	TYPE	eType;


	vec3<F32>	tSaveVectors[5];
	F32	tSaveFloats[2];
	bool	bSaved;
	F32   terHeightMin;
	std::vector<boost::function0<void> > _listeners;

public:
	Camera();
	virtual ~Camera() {}
	inline TYPE  getType()				 const	{return eType;}
	inline const vec3<F32>& getEye()	 const	{return vEye;}
	inline const vec3<F32>& getCenter()	 const	{return vCenter;}
	inline const vec3<F32>& getViewDir() const	{return vViewDir;}
	inline F32 getAngleX()				 const	{return fAngleX;}
	inline F32 getAngleY()				 const	{return fAngleY;}

	virtual bool load(const std::string& name) {return true;}
	virtual bool unload() {return true;}
	virtual void createCopy() {incRefCount();}
	virtual void removeCopy() {decRefCount();}

	///Add an event listener called after every RenderLookAt or RenderLookAtCube call
	virtual void addUpdateListener(boost::function0<void> f) {_listeners.push_back(f);}
	///Informs all listeners of a new event
	virtual void updateListeners();

public:
	void SaveCamera();
	void RestoreCamera();

	void Refresh();

	void setType(TYPE t)				{ eType = t;		Refresh(); }
	void setEye(vec3<F32> vPos)			{ vEye = vPos;		Refresh(); }
	void setCenter(vec3<F32> vPos)		{ vCenter = vPos;	Refresh(); }
	void setAngleX(F32 Angle)			{ fAngleX = Angle;	Refresh(); }
	void setAngleY(F32 Angle)			{ fAngleY = Angle;	Refresh(); }
	void setAngle(F32 AngleX, F32 AngleY)	{
		fAngleX = AngleX;
		fAngleY = AngleY;
		Refresh();
	}

	void Translate(vec3<F32> vector)	{ vEye += vector;	Refresh(); }
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

	void RenderLookAt(bool invertx = false, bool inverty=false, F32 planey=0.0f);

	void RenderLookAtToCubeMap(const vec3<F32>& eye, U8 nFace);

	inline F32 xfov_to_yfov(F32 xfov, F32 aspect) {
		xfov = RADIANS(xfov);
		F32 yfov = 2.0f * atan(tan(xfov * 0.5f) / aspect);
		return DEGREES(yfov);
	}

	inline F32 yfov_to_xfov(F32 yfov, F32 aspect) {
		yfov = RADIANS(yfov);
		F32 xfov = 2.0f * atan(tan(yfov * 0.5f) * aspect);
		return DEGREES(xfov);
	}

};
#endif

