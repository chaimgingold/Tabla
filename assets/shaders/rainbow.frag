#version 330

#ifdef GL_ES
precision mediump float;
#endif

#define PI 3.14159265359

in vec2 st;

out vec4 fragColor;

uniform sampler2D uTex0;
//uniform vec2 uResolution;

uniform float uTime;
uniform float uSeed;
uniform float uGradientFreq;
uniform vec2  uGradientCenter;


//float smoothedge(float v) {
//    return smoothstep(0.0, 1.0 / uResolution.x, v);
//}

float ring(vec2 p, float radius, float width) {
  return abs(length(p) - radius * 0.5) - width;
}


float plot(vec2 st, float pct){
  return  smoothstep( pct-0.02, pct, st.y) - 
          smoothstep( pct, pct+0.02, st.y);
}

vec3 replace(vec3 color, float mask, vec3 maskColor) {
    return (1.0-mask)*color+mask*maskColor;
}

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

void main() {
    float t = uSeed; //float(int(uTime));
//    float speed = hash11(t) * 8. + 1.;
	
//    vec2 gradientCenter = vec2(hash11(t), hash11(t+2.));
	
//    vec2 st = gl_FragCoord.xy/u_resolution;
    // Linear gradient
    // float theta = st.x;
    
    // Radial gradient
    float theta = distance(st, uGradientCenter);
	float phase = theta * PI*2. * uGradientFreq - uTime*8. - uGradientFreq;
    
    // Phase-shifted cosine gradients
	float minScale = .5;
    float r = rescale01(minScale, 1., cos01(phase*hash11(t+0.) + hash11(t+1.)));
    float g = rescale01(minScale, 1., cos01(phase*hash11(t+1.) + hash11(t+2.)));
    float b = rescale01(minScale, 1., cos01(phase*hash11(t+2.) + hash11(t+3.)));
    vec3 gradient = vec3(r,g,b);
    
    // Background color
    vec3 outcol = vec3(0,0,0);
//	vec3 outcol = vec3(0.050,0.038,0.055);
    // Plot rgb values
//    outcol = replace(outcol, plot(st, r), vec3(1.0,0.0,0.0));
//    outcol = replace(outcol, plot(st, g), vec3(0.0,1.0,0.0));
//    outcol = replace(outcol, plot(st, b), vec3(0.0,0.0,1.0));
	
    // Mask with texture
	float shapemask = texture(uTex0,st).a;
	
    outcol = replace(outcol, shapemask, gradient);
    
    fragColor = vec4(outcol,shapemask);
//	fragColor = vec4(st,0,1);
}

