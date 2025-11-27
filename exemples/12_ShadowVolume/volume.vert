#version 460

layout (location=0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;

out vec3 fPosition;

layout(location = 0) uniform mat4 mvMatrix;

void main()
{ 
	fPosition = (mvMatrix * vec4(vPosition,1.0)).xyz;
}