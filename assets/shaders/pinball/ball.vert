#version 330

#define M_PI 3.1415926535897932384626433832795

uniform mat4	ciModelView;
uniform mat4	ciModelViewProjection;
uniform mat3	ciNormalMatrix;
uniform mat4	ciViewMatrix;
uniform mat4	ciViewMatrixInverse;

uniform vec3 inBallLoc;
uniform vec3 inLightLoc;
uniform vec3 inEyeLoc;

in vec4 ciPosition;
in vec4 ciColor;
in vec3 ciNormal;
//in vec2 ciTexCoord0;

out vec4 color;

out highp vec3	NormalWorldSpace;
out highp vec3  EyeDirWorldSpace;
out highp vec3  LightVec; // reverse of it, in proper glsl style

void main()
{
	if (false)
	{
		// something in here wrong
		// -- basically, my crazy stack of transforms means that cinder's built in logic
		// for inferring these matrices and stuff is toast.
		vec4 positionViewSpace = ciModelView * ciPosition;
		vec4 eyeDirViewSpace = positionViewSpace - vec4( 0, 0, 0, 1 ); // eye is always at 0,0,0 in view space
		EyeDirWorldSpace = vec3( ciViewMatrixInverse * eyeDirViewSpace );
	}
	else
	{
		// mostly correct, except for renering balls to one another
		//EyeDirWorldSpace = vec3(0,0,1);
		EyeDirWorldSpace = normalize( inBallLoc - inEyeLoc ); // could move this out to c++
	}
	
	vec3 normalViewSpace = ciNormalMatrix * ciNormal;
	NormalWorldSpace = normalize( vec3( vec4( normalViewSpace, 0 ) * ciViewMatrix ) );

//	NormalWorldSpace = vec3(1,0,0);
	
	if (false)
	{
		NormalWorldSpace *= mat3(
			vec3(0,-1,0),
			vec3(1,0,0),
			vec3(0,0,-1)
		);
	}
	
	gl_Position = ciModelViewProjection * ciPosition;
	
//	coord = ciTexCoord0; // -1..1
	color = ciColor;
//	normal = vec3( vec4(ciNormal,1) * fixNormalMatrix );
//	gl_Position = ciModelViewProjection * ciPosition;

	LightVec = inLightLoc - inBallLoc;
}
