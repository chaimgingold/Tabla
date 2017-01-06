#version 330
#define PI 3.14159265359
in vec2 pos;
//in vec4 color;

out vec4 fragColor;


uniform float uTime;
uniform float uPartyProgress;
uniform vec2  uPartyLoc;

float cos01(float phase) {
	return cos(phase) * 0.5 + 0.5;
}

float rescale01(float low, float high, float value) {
	return low + value*(high - low);
}

// Dave Hoskins Hash without Sine
// https://www.shadertoy.com/view/4djSRW
#define HASHSCALE1 .1031
float hash11(float p) {
	vec3 p3  = fract(vec3(p) * HASHSCALE1);
	p3 += dot(p3, p3.yzx + 19.19);
	return fract((p3.x + p3.y) * p3.z);
}


void main()
{

	float gradientFreq = 5;
	vec2 gradientCenter = vec2(0.5);
	float gradientSpeed = 1;
	float gridBrightness = .55;
	if (uPartyProgress > 0) {
		gridBrightness = 1;
		gradientCenter = uPartyLoc;
		gradientFreq = 10;
		gradientSpeed = 15;
	}




	float uSeed = 1;
	vec2 st = gl_FragCoord.xy / vec2(1024, 768);


	vec2 usepos;
	float gridSize;

	if (false) {
		// world space
		usepos = pos;
		gridSize = 3;
	} else {
		// pixel space
		usepos = gl_FragCoord.xy;
		gridSize = 20;
	}

//	float x = smoothstep(0.2, 0, fract(usepos.x/gridSize));
//	float y = smoothstep(0.2, 0, fract(usepos.y/gridSize));
	vec2 xy = fract(usepos/vec2(gridSize,gridSize));
	float k =.05;
	if (xy.x>k) xy.x=0; else xy.x=1;
	if (xy.y>k) xy.y=0; else xy.y=1;
	float gridMask = max(xy.x,xy.y) * gridBrightness;
	vec3 gridColor = vec3(0.1,0.9,0.4);
	vec3 bgColor = vec3(0.0,0.0,0.0);


	float t = uSeed;

//	gradientCenter = vec2(hash11(t), hash11(t+2.));

	// Radial gradient
	float theta = distance(st, gradientCenter);
	float phase = theta * PI*2. * gradientFreq - uTime*gradientSpeed - gradientFreq;

	// Phase-shifted cosine gradients
	float minScale = .5;
	vec3 gradient = vec3( rescale01(minScale, 1., cos01(phase*hash11(t+0.) + hash11(t+1.)))
						, rescale01(minScale, 1., cos01(phase*hash11(t+1.) + hash11(t+2.)))
						, rescale01(minScale, 1., cos01(phase*hash11(t+2.) + hash11(t+3.)))
						);
	
	if (uPartyProgress>0.) gradient = pow( gradient, vec3(2.) );

	// Background color

	fragColor = vec4(mix(bgColor, gradient, gridMask), 1);
//	fragColor = vec4(0,.8,.8,1);
}
