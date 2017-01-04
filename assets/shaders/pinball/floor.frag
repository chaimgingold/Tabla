#version 330

in vec2 pos;
//in vec4 color;

out vec4 fragColor;

void main()
{
//	vec2 usepos = pos;
//	const float gridSize = 3;
	vec2 usepos = gl_FragCoord.xy;
	const float gridSize = 20;
	
	float x = smoothstep(0.1, 0, fract(usepos.x/gridSize));
	float y = smoothstep(0.1, 0, fract(usepos.y/gridSize));
	float gridMask = (x+y)*0.5;
	vec3 gridColor = vec3(0.1,0.9,0.4);
	vec3 bgColor = vec3(0.0,0.1,0.1);
	
	fragColor = vec4(mix(bgColor, gridColor, gridMask), 1);
//	fragColor = vec4(0,.8,.8,1);
}
