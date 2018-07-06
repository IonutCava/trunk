/*“Copyright 2009-2013 DIVIDE-Studio”*/
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

#include "core.h"
#include "Core/Resources/Headers/Resource.h"

class Camera : public Resource {

public:	
	enum CameraType {
		FREE_FLY, 
		FIRST_PERSON,
		THIRD_PERSON,
		ORBIT,
		SCRIPTED
	};

public:
	Camera(CameraType type);
	virtual ~Camera() {}

	void Refresh();
	void SaveCamera();
	void RestoreCamera();

	void MoveForward(F32 factor);	
	void MoveStrafe(F32 factor);
	void TranslateForward(F32 factor);
	void TranslateStrafe(F32 factor);	
	void MoveAnaglyph(F32 factor);

	inline CameraType  getType()		 const	{return _type;}
	inline const vec3<F32>& getEye()	 const	{return _eye;}
	inline const vec3<F32>& getCenter()	 const	{return _center;}
	inline const vec3<F32>& getViewDir() const	{return _view;}
	inline F32 getAngleX()				 const	{return _angleX;}
	inline F32 getAngleY()				 const	{return _angleY;}

	void setType(CameraType t)			    {_type = t;		   Refresh();}
	void setEye(vec3<F32> vPos)			    {_eye = vPos;	   Refresh();}
	void setCenter(vec3<F32> vPos)		    {_center = vPos;   Refresh();}
	void setAngleX(F32 Angle)			    {_angleX = Angle;  Refresh();}
	void setAngleY(F32 Angle)			    {_angleY = Angle;  Refresh();}
	void setAngle(F32 AngleX, F32 AngleY)	{_angleX = AngleX; _angleY = AngleY; Refresh();}
	void Translate(vec3<F32> vector)	    {_eye += vector;   Refresh();}
	
	void RotateX(F32 AngleX){
		_angleX += AngleX;
		if(_angleX<0.0f) _angleX += M_PI*2;
		else if(_angleX>M_PI*2)	_angleX -= M_PI*2;
		Refresh();
	}

	void RotateY(F32 AngleY){
		_angleY += AngleY;
		if(_angleY<0.0f || _angleY>M_PI) _angleY -= AngleY;
		Refresh();
	}

	void Rotate(F32 AngleX, F32 AngleY){
		_angleX += AngleX;
		_angleY += AngleY;
		if(_angleX<0.0f) _angleX += M_PI*2;
		else if(_angleX>M_PI*2)	_angleX -= M_PI*2;
		if(_angleY<0 || _angleY>M_PI) _angleY -= AngleY;
		Refresh();
	}

	inline F32 xfov_to_yfov(F32 xfov, F32 aspect) {
		return DEGREES(2.0f * atan(tan(RADIANS(xfov) * 0.5f) / aspect));
	}

	inline F32 yfov_to_xfov(F32 yfov, F32 aspect) {
		return DEGREES(2.0f * atan(tan(RADIANS(yfov) * 0.5f) * aspect));
	}

	void RenderLookAt(bool invertx = false, bool inverty=false, F32 planey=0.0f);
	///Position the camera so that it faces a side of a cube with a 90 degree FOV
	void RenderLookAtToCubeMap(const vec3<F32>& eye, U8 nFace);
	///Nothing really to unload
	virtual bool unload() {return true;}
	///Add an event listener called after every RenderLookAt or RenderLookAtCube call
	virtual void addUpdateListener(boost::function0<void> f) {_listeners.push_back(f);}
	///Informs all listeners of a new event
	virtual void updateListeners();

protected:

	vec3<F32>	_eye;
	vec3<F32>	_center;
	vec3<F32>	_view;
	vec3<F32>	_left;	
	vec3<F32>	_up;
	vec3<F32>	_savedVectors[5];

	F32 _angleX;
	F32	_angleY;	
	F32	_savedFloats[2];

	bool _saved;

	CameraType	_type;

	vectorImpl<boost::function0<void> > _listeners;
};
#endif

