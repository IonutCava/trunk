

void phong_directionalLight(in int _light, 
							in vec4 _specularIn, 
							in vec3 LightVector, //normalized value
							in vec3 NormalVector,//normalized value
							in vec3 EyeVector,   //normalized value
							inout vec4 _ambient,
							inout vec4 _diffuse,
							inout vec4 _specular)
{
	///Lambert term for the position vector
	iDiffuse = max(dot(LightVector, NormalVector), 0.0);
	//Specular intensity based on material shininess
	float iSpecular = pow(clamp(dot(reflect(-LightVector, NormalVector), EyeVector), 0.0, 1.0), material[3].x );

	//add the lighting contributions
	_ambient  += gl_LightSource[_light].ambient * material[0];
	_diffuse  += gl_LightSource[_light].diffuse * material[1] *  iDiffuse;
	_specular += gl_LightSource[_light].specular * _specularIn * iSpecular;
}