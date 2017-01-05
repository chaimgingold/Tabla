#version 330

uniform samplerCube uCubeMapTex;
in vec3	NormalWorldSpace;
in vec3 EyeDirWorldSpace;
uniform vec4 uLightColor;

//in vec2 coord;
in vec4 color;
in vec3 LightVec;
//in vec3 normal;

out vec4 fragColor;

const float aaConstant = .1;

void main()
{
	vec3 normalizedNormalWS = normalize(NormalWorldSpace);
	
	vec3 reflectedEyeWorldSpace = reflect( EyeDirWorldSpace, normalizedNormalWS );
	
//	reflectedEyeWorldSpace *= -1;
	
//	reflectedEyeWorldSpace = reflectedEyeWorldSpace.zxy;
	
//	reflectedEyeWorldSpace = vec3(
//		reflectedEyeWorldSpace.y,
//		reflectedEyeWorldSpace.z,
//		reflectedEyeWorldSpace.x
//		);
	
	fragColor = texture( uCubeMapTex, reflectedEyeWorldSpace );
	
	float shiny = pow( max( 0, dot(normalizedNormalWS, normalize(LightVec)) ), 8 );
	
	fragColor = min( fragColor + uLightColor*shiny, vec4(1,1,1,1) );
//	fragColor = vec4(NormalWorldSpace,1);
//	fragColor = vec4(EyeDirWorldSpace,1);
//	fragColor = vec4(reflectedEyeWorldSpace,1);
	
//	fragColor = vec4(1,1,1,1);
}