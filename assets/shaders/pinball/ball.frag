#version 330

uniform samplerCube uCubeMapTex;
in vec3	NormalWorldSpace;
in vec3 EyeDirWorldSpace;
uniform vec4 uLightColor;

in vec4 color;
in vec3 LightVec;

out vec4 fragColor;

vec4 ambLight = vec4(.2,.15,.15,1);

void main()
{
	vec3 normalizedNormalWS = normalize(NormalWorldSpace);
	
	vec3 reflectedEyeWorldSpace = reflect( EyeDirWorldSpace, normalizedNormalWS );
	
	fragColor = texture( uCubeMapTex, reflectedEyeWorldSpace );
	
	if (true)
	{
		float shiny = 1.2 * pow( max( 0, dot(reflectedEyeWorldSpace, normalize(LightVec)) ), 4.5 );
		
		fragColor = fragColor + uLightColor*shiny + ambLight;
	}
//	fragColor = vec4(NormalWorldSpace,1);
//	fragColor = vec4(EyeDirWorldSpace,1);
//	fragColor = vec4(reflectedEyeWorldSpace,1);
	
//	fragColor = vec4(1,1,1,1);
}