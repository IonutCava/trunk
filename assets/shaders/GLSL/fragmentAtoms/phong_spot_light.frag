const float cos_outer_cone_angle = 0.8; // 36 degrees

void phong_spotLight(in int _light, 
					 in vec4 _specularIn, 
					 in vec3 LightVector, //normalized value
					 in vec3 NormalVector,//normalized value
					 in vec3 EyeVector,   //normalized value
					 inout vec4 _ambient,
					 inout vec4 _diffuse,
					 inout vec4 _specular)
{
	//get the distance from surface to light
	float distance = length(vPixToEyeTBN[_light]);
	vec3  spotDirection =  normalize(gl_LightSource[_light].spotDirection);
	float cos_cur_angle = dot(-LightVector, spotDirection);
	float cos_inner_cone_angle = gl_LightSource[_light].spotCosCutoff;
	float cos_inner_minus_outer_angle =  cos_inner_cone_angle - cos_outer_cone_angle;
	float spot = 0.0;
	spot = clamp((cos_cur_angle - cos_outer_cone_angle) /  cos_inner_minus_outer_angle, 0.0, 1.0);
	//Diffuse intensity
	iDiffuse = max(dot(LightVector, NormalVector), 0.0);
	float spotEffect;
	if(iDiffuse > 0.0){
		spotEffect = pow(spotEffect, gl_LightSource[_light].spotExponent);
		float attenuation = spotEffect/(gl_LightSource[_light].constantAttenuation + 
								  gl_LightSource[_light].linearAttenuation * distance + 
							      gl_LightSource[_light].quadraticAttenuation * distance * distance);
		attenuation = max(attenuation, 0.0);
		//Specular intensity based on material shininess
		float iSpecular = pow(clamp(dot(reflect(-LightVector, NormalVector), EyeVector), 0.0, 1.0), material[3].x );
		//add the lighting contributions
		_ambient += material[0] * gl_LightSource[_light].ambient * attenuation;;
		_diffuse += material[1] * gl_LightSource[_light].diffuse * iDiffuse * attenuation;;
		_specular += _specularIn * gl_LightSource[_light].specular * iSpecular * attenuation;
		
	}
}