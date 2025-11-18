#version 460 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in float randomOrientation;

out float vRandomOrientation;

void main()
{
     gl_Position = vPosition; // Sans la projection
     vRandomOrientation = randomOrientation;
}

