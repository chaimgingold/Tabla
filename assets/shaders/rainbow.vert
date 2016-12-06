#version 330

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec2 ciTexCoord0;

out vec2 st;

void main()
{
	st = ciTexCoord0;
	gl_Position = ciModelViewProjection * ciPosition;
}