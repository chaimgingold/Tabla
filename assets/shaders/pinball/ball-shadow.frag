#version 330

in vec2 coord;
in vec4 color;

out vec4 fragColor;

const float aaConstant = .1;

void main()
{
	float r = length( coord - vec2(.5,.5) ) * 2;

//	vec4 c = color;
	vec4 c = vec4( vec3(1,1,1) * .2, 1 );

	c.a *= smoothstep( 1.0, 1.0 - aaConstant, r);
	
	fragColor = c;
}
