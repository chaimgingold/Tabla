#version 330

//uniform sampler2D uTex0;
//uniform float     uAspectRatio = 1.33333;
//uniform float	  uTime;
//uniform float	  uPhase;

in vec2 coord;
in vec4 color;

out vec4 fragColor;

const float aaConstant = .2;

void main()
{
	float r = length( coord - vec2(.5,.5) ) * 2;

	vec4 c = color;

	c.a = smoothstep( 1.0, 1.0 - aaConstant, r);
	
	fragColor = c;
}
