#version 330

uniform mat4 ciModelViewProjection;

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
	normal = ciNormal;
	gl_Position = ciModelViewProjection * ciPosition;
}
