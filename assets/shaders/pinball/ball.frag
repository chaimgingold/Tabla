#version 330

//in vec2 coord;
in vec4 color;
in vec3 normal;

out vec4 fragColor;

const float aaConstant = .1;

void main()
{
	vec3 norm = normalize(normal); // need this? i think so for proper phong
	
	float r = 0.; //length( coord - vec2(.5,.5) ) * 2;

	vec4 c = color;

	c.a *= smoothstep( 1.0, 1.0 - aaConstant, r);
	
	fragColor = c;
	fragColor = vec4( norm, 1. );
}