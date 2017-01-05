#version 330

uniform float uTime;
uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec4 ciColor;
in vec3 ciNormal;

//in vec2 ciTexCoord0;

out vec2 coord;
out vec4 color;
out vec3 normal;

mat3 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c );
}

void main()
{
//	coord = ciTexCoord0; // -1..1
	color = ciColor;
	normal = ciNormal;
	
	normal = rotationMatrix( vec3(0,0,-1), uTime*.2 ) * normal;
	
	gl_Position = ciModelViewProjection * ciPosition;
}
