#version 330

//in vec2 coord;
in vec4 color;
in vec3 normal;

out vec4 fragColor;

void main()
{
	fragColor = mix( vec4( normal, 1 ) * color, color, .5 );
}
