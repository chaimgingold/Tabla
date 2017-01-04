#version 330

uniform samplerCube uCubeMapTex;
in vec3	NormalWorldSpace;
in vec3 EyeDirWorldSpace;

//in vec2 coord;
in vec4 color;
//in vec3 normal;

out vec4 fragColor;

const float aaConstant = .1;

void main()
{
	vec3 reflectedEyeWorldSpace = reflect( EyeDirWorldSpace, normalize(NormalWorldSpace) );
	
//	reflectedEyeWorldSpace *= -1;
	
//	reflectedEyeWorldSpace = reflectedEyeWorldSpace.zxy;
	
//	reflectedEyeWorldSpace = vec3(
//		reflectedEyeWorldSpace.y,
//		reflectedEyeWorldSpace.z,
//		reflectedEyeWorldSpace.x
//		);
	
	fragColor = texture( uCubeMapTex, reflectedEyeWorldSpace );

//	fragColor = vec4(NormalWorldSpace,1);
//	fragColor = vec4(EyeDirWorldSpace,1);
//	fragColor = vec4(reflectedEyeWorldSpace,1);
	
//	fragColor = vec4(1,1,1,1);
}