#version 460

layout(location = 0) uniform mat4 MVMatrix;
layout(location = 1) uniform mat4 ProjMatrix;
layout(location = 2) uniform mat3 normalMatrix;

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec3 vNormal;

out vec3 fNormal;
out vec3 fPosition;

void main()
{
     vec4 vEyeCoord = MVMatrix * vPosition;
     gl_Position = ProjMatrix * vEyeCoord;

     fPosition = vEyeCoord.xyz;
     fNormal = normalMatrix * vNormal;
}
