#version 330

in vec2 vUV;

out vec4 fragColor;

uniform float uTime;

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

void main() {
	vec3 color = hsv2rgb_smooth(vec3(
									 vUV.y + uTime/10 + vUV.x * 0.05,
									 0.8,
									 0.6 + vUV.x * 0.1
									 ));
	// vec3 color = vec3(1,1,1);
	fragColor = vec4(color,1.0);
}

