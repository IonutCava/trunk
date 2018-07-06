#include "Light.h"
#include "Hardware/Video/GFXDevice.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Managers/CameraManager.h"
#include "Managers/ResourceManager.h"
#include "Utility/Headers/ParamHandler.h"
#include "Rendering/Frustum.h"
#include "Rendering/common.h"

Light::Light(U8 slot, F32 radius) : _slot(slot), _radius(radius)
{
	vec4 _white(1.0f,1.0f,1.0f,1.0f);
	vec2 angle = vec2(0.0f, RADIANS(45.0f));
	vec4 position = vec4(-cosf(angle.x) * sinf(angle.y),-cosf(angle.y),	-sinf(angle.x) * sinf(angle.y),	0.0f );
	vec4 diffuse = _white.lerp(vec4(1.0f, 0.5f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 0.8f, 1.0f), 0.25f + cosf(angle.y) * 0.75f);

	_lightProperties["position"] = position;
	_lightProperties["ambient"] = _white;
	_lightProperties["diffuse"] = diffuse;
	_lightProperties["specular"] = diffuse;
	char lightName[9];
	sprintf(lightName, "Light %d", slot);
	_light = ResourceManager::getInstance().LoadResource<Sphere3D>(lightName);
	_light->getResolution() = 8;
	_light->getSize() = radius;
	GFXDevice::getInstance().createLight(_slot);
	
}

Light::~Light()
{
	_lightProperties.empty();
	ResourceManager::getInstance().remove(_light);
}
void Light::update()
{
	_light->getTransform()->setPosition(_lightProperties["position"]);
	_light->getTransform()->scale(vec3(1, 1 ,1));
	_light->getSize() = _radius;
	_light->getTransform()->rotateEuler(vec3(0, 0 ,0));
	_light->getMaterial().diffuse = getDiffuseColor();
	if(!GFXDevice::getInstance().getDeferredShading())
		GFXDevice::getInstance().setLight(_slot,_lightProperties);
}

void Light::setLightProperties(const std::string& name, vec4 values)
{
	std::tr1::unordered_map<std::string,vec4>::iterator it = _lightProperties.find(name);
	if (it != _lightProperties.end())
		_lightProperties[name] = values;

	if(name.compare("spotDirection") == 0)
		_lightProperties["spotDirection"] = values;
}

void Light::draw()
{
	GFXDevice::getInstance().drawSphere3D(_light);
}


void Light::getWindowRect(U16 & x, U16 & y, U16 & width, U16 & height)
{
	ParamHandler& par =  ParamHandler::getInstance();

	Frustum::getInstance().Extract(CameraManager::getInstance().getActiveCamera()->getEye());
	mat4& cameraViewMatrix = Frustum::getInstance().getModelviewMatrix();
	U16 windowWidth = Engine::getInstance().getWindowDimensions().x;
	U16 windowHeight = Engine::getInstance().getWindowDimensions().y;
	F32 left=-1.0f;	F32 right=1.0f;
	F32 bottom=-1.0f; F32 top=1.0f;
	vec3 l=cameraViewMatrix*vec3(_lightProperties["position"]);
	F32 halfNearPlaneHeight=par.getParam<D32>("zNear")*(F32)tan(D32(par.getParam<F32>("FOV")*M_PI/360));
	F32 halfNearPlaneWidth=halfNearPlaneHeight*par.getParam<F32>("aspectRatio");


	//IONUT: Source found at: Paul's Projects - Deffered Shading 
	//   http://www.paulsprojects.net/opengl/defshad/defshad.html
	//plane normal. Of the form (x, 0, z)
	vec3 normal;

	//Calculate the discriminant of the quadratic we wish to solve to find nx(divided by 4)
	F32 d=(l.z*l.z) * ( (l.x*l.x) + (l.z*l.z) - _radius*_radius );

	//If d>0, solve the quadratic to get the normal to the plane
	if(d>0.0f)
	{
		F32 rootD=(F32)sqrt(d);

		//Loop through the 2 solutions
		for(U8 i=0; i<2; ++i)
		{
			//Calculate the normal
			if(i==0)
				normal.x=_radius*l.x+rootD;
			else
				normal.x=_radius*l.x-rootD;

			normal.x/=(l.x*l.x + l.z*l.z);

			normal.z=_radius-normal.x*l.x;
			normal.z/=l.z;

			//We need to divide by normal.x. If ==0, no good
			if(normal.x==0.0f)
				continue;


			//p is the point of tangency
			vec3 p;

			p.z=(l.x*l.x) + (l.z*l.z) - _radius*_radius;
			p.z/=l.z-((normal.z/normal.x)*l.x);

			//If the point of tangency is behind the camera, no good
			if(p.z>=0.0f)
				continue;

			p.x=-p.z * normal.z/normal.x;

			//Calculate where the plane meets the near plane
			//divide by the width to give a value in [-1, 1] for values on the screen
			F32 screenX=normal.z * par.getParam<D32>("zNear") / (normal.x*halfNearPlaneWidth);
			
			//If this is a left bounding value (p.x<l.x) and is further right than the
			//current value, update
			if(p.x<l.x && screenX>left)
				left=screenX;

			//Similarly, update the right value
			if(p.x>l.x && screenX<right)
				right=screenX;
		}
	}


	//Repeat for planes parallel to the x axis
	//normal is now of the form(0, y, z)
	normal.x=0.0f;

	//Calculate the discriminant of the quadratic we wish to solve to find ny(divided by 4)
	d=(l.z*l.z) * ( (l.y*l.y) + (l.z*l.z) - _radius*_radius );

	//If d>0, solve the quadratic to get the normal to the plane
	if(d>0.0f)
	{
		F32 rootD=(F32)sqrt(d);

		//Loop through the 2 solutions
		for(U8 i=0; i<2; ++i)
		{
			//Calculate the normal
			if(i==0)
				normal.y=_radius*l.y+rootD;
			else
				normal.y=_radius*l.y-rootD;

			normal.y/=(l.y*l.y + l.z*l.z);

			normal.z=_radius-normal.y*l.y;
			normal.z/=l.z;

			//We need to divide by normal.y. If ==0, no good
			if(normal.y==0.0f)
				continue;


			//p is the point of tangency
			vec3 p;

			p.z=(l.y*l.y) + (l.z*l.z) - _radius*_radius;
			p.z/=l.z-((normal.z/normal.y)*l.y);

			//If the point of tangency is behind the camera, no good
			if(p.z>=0.0f)
				continue;

			p.y=-p.z * normal.z/normal.y;

			//Calculate where the plane meets the near plane
			//divide by the height to give a value in [-1, 1] for values on the screen
			F32 screenY=normal.z * par.getParam<D32>("zNear") / (normal.y*halfNearPlaneHeight);
			
			//If this is a bottom bounding value (p.y<l.y) and is further up than the
			//current value, update
			if(p.y<l.y && screenY>bottom)
				bottom=screenY;

			//Similarly, update the top value
			if(p.y>l.y && screenY<top)
				top=screenY;
		}
	}

	//Now scale and bias into [0, windowWidth-1]x[0, windowHeight-1]
	U16 x1=U16((left+1)*windowWidth/2);
	U16 x2=U16((right+1)*windowWidth/2);

	U16 y1=U16((bottom+1)*windowHeight/2);
	U16 y2=U16((top+1)*windowHeight/2);

	//Clamp these values to the edge of the screen
	if(x1<0)
		x1=0;
	
	if(x2>windowWidth-1)
		x2=windowWidth;

	if(y1<0)
		y1=0;

	if(y2>windowHeight-1)
		y2=windowHeight;

	//Fill the outputs
	x=x1;
	width=x2-x1;

	y=y1;
	height=y2-y1;
}