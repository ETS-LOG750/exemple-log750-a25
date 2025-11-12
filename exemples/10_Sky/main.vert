#version 460 core

uniform mat4 mvMatrix;
uniform mat4 projMatrix;

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec3 vNormal;

out vec3 fNormalWorld;
out vec3 fPositionWorld;

void main()
{
     vec4 vEyeCoord = mvMatrix * vPosition;
     gl_Position = projMatrix * vEyeCoord;

     // Direction de vue (pour la reflexion)
     // en espace monde
     fPositionWorld = vPosition.xyz;
     fNormalWorld = vNormal; 
}

