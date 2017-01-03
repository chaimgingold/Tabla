#version 330

uniform mat4 ciModelViewProjection;
uniform mat4 fixNormalMatrix;

in vec4 ciPosition;
in vec4 ciColor;
in vec3 ciNormal;
//in vec2 ciTexCoord0;

out vec2 coord;
out vec4 color;
out vec3 normal;

void main()
{
//	coord = ciTexCoord0; // -1..1
	color = ciColor;
	normal = vec3( vec4(ciNormal,1) * fixNormalMatrix );
	gl_Position = ciModelViewProjection * ciPosition;
}
