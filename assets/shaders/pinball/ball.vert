#version 330

uniform mat4	ciModelView;
uniform mat4	ciModelViewProjection;
uniform mat3	ciNormalMatrix;
uniform mat4	ciViewMatrix;
uniform mat4	ciViewMatrixInverse;

in vec4 ciPosition;
in vec4 ciColor;
in vec3 ciNormal;
//in vec2 ciTexCoord0;

//out vec2 coord;
out vec4 color;

out highp vec3	NormalWorldSpace;
out highp vec3  EyeDirWorldSpace;

void main()
{
	vec4 positionViewSpace = ciModelView * ciPosition;
	vec4 eyeDirViewSpace = positionViewSpace - vec4( 0, 0, 0, 1 ); // eye is always at 0,0,0 in view space
	EyeDirWorldSpace = vec3( ciViewMatrixInverse * eyeDirViewSpace );
	
	vec3 normal = ciNormal;
//	vec3 normal = vec3( vec4(ciNormal,1) * fixNormalMatrix );
	
	vec3 normalViewSpace = ciNormalMatrix * normal;
	NormalWorldSpace = normalize( vec3( vec4( normalViewSpace, 0 ) * ciViewMatrix ) );
	gl_Position = ciModelViewProjection * ciPosition;
	
//	coord = ciTexCoord0; // -1..1
	color = ciColor;
//	normal = vec3( vec4(ciNormal,1) * fixNormalMatrix );
//	gl_Position = ciModelViewProjection * ciPosition;
}
