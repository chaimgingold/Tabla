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

	vec4 bkgnd;
//	bkgnd.r = vertTexCoord0.x;
//	bkgnd.g = vertTexCoord0.y;
//	bkgnd.b = 1.;
//	bkgnd.a = 1.;
// ...nice gradient, but alpha makes more sense overall.
	bkgnd = vec4(0,0,0,0);
	
	//
	float playhead = uPhase;

    vec2 xy = vertTexCoord0;

	float gradient = smoothstep(playhead-0.1, playhead, xy.x);
	gradient = pow(gradient,3);
	if (xy.x > playhead) {
		gradient = 0.;
	}
	
	vec3 image = texture(uTex0, xy).xyz;


	vec4 rainbow = vec4(colorfield(xy),1);
	
	vec4 beamColor;
	
	if ( (image.r + image.g + image.b)/3 < 0.2 ) {
	
		// off
//		beamColor = vec4(colorfield(xy),1);
//		fragColor = vec4(1,0,0,1);
//		fragColor = vec4(0,0,0,1);
		fragColor = rainbow;
		beamColor = vec4(1,1,0,1);
		
//		return;
		
//		fragColor = bkgnd ; //vec4(colorfield(xy), 1.);

//		fragColor = mix( fragColor*.25, fragColor, gradient );
		
//		if (xy.x < playhead) {
//			vec3 inverted = vec3(1.-fragColor.r, 1.-fragColor.g, 1.-fragColor.b);

//			fragColor = mix(fragColor, vec4(inverted*2., 1.), gradient);
//		}
	} else {
	
		// on
//		beamColor = vec4(colorfield(xy),1);
//		fragColor = vec4(0.,1.,0.,1.);
//		return;

//		beamColor = rainbow; //vec4( 1.-beamColor.x, 1.-beamColor.y, 1.-beamColor.z, 1. );
		
		beamColor = vec4(0,1,1,.9);
		fragColor = bkgnd; //vec4(0., 0., 0., 0.);


//			fragColor = gradient * vec4(colorfield(xy), 1.);
//			fragColor.b += gradient;
//			fragColor.g += gradient;
//			fragColor.a += gradient;
//		}
	}
	
	
	fragColor = mix( fragColor, beamColor, gradient );

}













