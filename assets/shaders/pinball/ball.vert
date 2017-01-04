#version 330

#define M_PI 3.1415926535897932384626433832795

uniform mat4	ciModelView;
uniform mat4	ciModelViewProjection;
uniform mat3	ciNormalMatrix;
uniform mat4	ciViewMatrix;
uniform mat4	ciViewMatrixInverse;

in vec4 ciPosition;
in vec4 ciColor;
in vec3 ciNormal;
//in vec2 ciTexCoord0;

//out vec2 coord;
out vec4 color;

out highp vec3	NormalWorldSpace;
out highp vec3  EyeDirWorldSpace;

mat4 rotationMatrix(vec3 axis, float angle)
{
//    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

void main()
{
	vec4 positionViewSpace = ciModelView * ciPosition;
	vec4 eyeDirViewSpace = positionViewSpace - vec4( 0, 0, 0, 1 ); // eye is always at 0,0,0 in view space
	EyeDirWorldSpace = vec3( ciViewMatrixInverse * eyeDirViewSpace );
	
	vec3 normal = ciNormal;
	vec3 normalViewSpace = ciNormalMatrix * normal;
	NormalWorldSpace = normalize( vec3( vec4( normalViewSpace, 0 ) * ciViewMatrix ) );

	if (false)
	{
		NormalWorldSpace *= mat3(
			vec3(0,-1,0),
			vec3(1,0,0),
			vec3(0,0,-1)
		);
	}
	
	gl_Position = ciModelViewProjection * ciPosition;
	
//	coord = ciTexCoord0; // -1..1
	color = ciColor;
//	normal = vec3( vec4(ciNormal,1) * fixNormalMatrix );
//	gl_Position = ciModelViewProjection * ciPosition;
}
