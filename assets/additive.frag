#version 330

uniform sampler2D uTex0;
uniform float     uAspectRatio = 1.33333;
uniform float	  uTime;
uniform float	  uPhase;

in vec2 vertTexCoord0;

out vec4 fragColor;


float noise(float x, float time) {
    float amplitude = 1.;
    float frequency = 1.;
    float y = sin(x * frequency);
    float t = 0.01*(-time*130.0);
    y += sin(x*frequency*2.1 + t)*4.5;
    y += sin(x*frequency*1.72 + t*1.121)*4.0;
    y += sin(x*frequency*2.221 + t*0.437)*5.0;
    y += sin(x*frequency*3.1122+ t*4.269)*2.5;
    y *= amplitude*0.06;
    return y;
}

vec3 colorfield(vec2 xy) {
    float r = noise(xy.y*2., uTime*0.1+xy.y*10. + noise(xy.x*19., uTime*0.2)*10.) * 0.5 + 0.5;
    float g = noise(xy.y*1., uTime*0.3+xy.y*30. + noise(xy.x*10., uTime*0.1)*30.) * 0.5 + 0.5;
    float b = noise(xy.y*3., uTime*0.4+xy.y*20. + noise(xy.x*15., uTime*0.3)*20.) * 0.5 + 0.5;
    vec3 color = vec3(r,g,b);
    return color;
}

void main() {


//	float trash = uTime + uPhase + texture(uTex0, vec2(0,0) ).x;
//	fragColor.r = vertTexCoord0.x;
//	fragColor.g = vertTexCoord0.y;
//	fragColor.b = 0.;
//	fragColor.a = 1.;
//	return;
	
	//
	float playhead = uPhase;

    vec2 xy = vertTexCoord0;

	float gradient = smoothstep(playhead-0.1, playhead, xy.x);

	vec3 image = texture(uTex0, xy).xyz;
	
	if ( (image.r + image.g + image.b)/3 < 0.5 ) {
		fragColor = vec4(colorfield(xy), 1.);

//		fragColor = mix( fragColor*.25, fragColor, gradient );
		
		if (xy.x < playhead) {
			vec3 inverted = vec3(1.-fragColor.r, 1.-fragColor.g, 1.-fragColor.b);

			fragColor = mix(fragColor, vec4(inverted*2., 1.), gradient);
		}
	} else {
		fragColor = vec4(0., 0., 0., 0.);

		if (xy.x < playhead) {
			fragColor.b += gradient;
			fragColor.g += gradient;
			fragColor.a += gradient;
		}
	}
}













