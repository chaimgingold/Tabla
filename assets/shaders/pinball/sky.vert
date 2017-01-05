#version 330

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
//in vec4 ciColor;
//in vec2 ciTexCoord0;

out vec2 pos;
//out vec4 color;

void main()
{
//	coord = ciTexCoord0; // -1..1
//	color = ciColor;
	gl_Position = ciModelViewProjection * ciPosition;
	pos = ciPosition.xy;
}
