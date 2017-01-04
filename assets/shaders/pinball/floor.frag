#version 330

//in vec2 coord;
//in vec4 color;

out vec4 fragColor;

void main()
{
	const float gridSize = 20;
	float x = smoothstep(0.1, 0, fract(gl_FragCoord.x/gridSize));
	float y = smoothstep(0.1, 0, fract(gl_FragCoord.y/gridSize));
	float gridMask = (x+y)*0.5;
	vec3 gridColor = vec3(0.1,0.9,0.4);
	vec3 bgColor = vec3(0.0,0.1,0.1);
	
	fragColor = vec4(mix(bgColor, gridColor, gridMask), 1);
}
