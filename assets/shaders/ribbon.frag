#version 330
#define PI 3.14159265359
in vec2 vUV;
in float vAlpha;

out vec4 fragColor;

uniform float uTime;
uniform float uSeed;

// Dave Hoskins Hash without Sine
// https://www.shadertoy.com/view/4djSRW
#define HASHSCALE1 .1031
float hash11(float p) {
	vec3 p3  = fract(vec3(p) * HASHSCALE1);
	p3 += dot(p3, p3.yzx + 19.19);
	return fract((p3.x + p3.y) * p3.z);
}

// Smooth HSV to RGB conversion (iq)
vec3 hsv2rgb_smooth( in vec3 c )
{
	vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );

	rgb = rgb*rgb*(3.0-2.0*rgb); // cubic smoothing

	return c.z * mix( vec3(1.0), rgb, c.y);
}


float cos01(float phase) {
    return cos(phase) * 0.5 + 0.5;
}

float rescale01(float low, float high, float value) {
    return low + value*(high - low);
}


vec3 hsvStyle() {
	return hsv2rgb_smooth(vec3(
						vUV.y + uTime/10 + vUV.x * 0.05,
						0.8,
						0.6 + vUV.x * 0.1
						));
}

vec3 cosineGradStyle() {
//	float s = uSeed;
	float t = uSeed;
	float minScale = 0.1;
	float phase = vUV.y + uTime;
//	float theta = vUV.y/100.0;
//	float uGradientFreq = 1;
//	float phase = theta * PI*2. * uGradientFreq - t - uGradientFreq;
//
//	float r = rescale01(minScale, 1., cos01(phase*hash11(t+0.) + hash11(t+1.)));
//	float g = rescale01(minScale, 1., cos01(phase*hash11(t+1.) + hash11(t+2.)));
//	float b = rescale01(minScale, 1., cos01(phase*hash11(t+2.) + hash11(t+3.)));
	float r = cos01(phase*hash11(t+0.) + hash11(t+1.));
	float g = cos01(phase*hash11(t+1.) + hash11(t+2.));
	float b = cos01(phase*hash11(t+2.) + hash11(t+3.));
	vec3 gradient = vec3(r,g,b);
	return gradient;
}

void main() {


//	vec3 color = hsvStyle();
	vec3 color = cosineGradStyle();

	// vec3 color = vec3(1,1,1);
	fragColor = vec4(color,vAlpha);
}

