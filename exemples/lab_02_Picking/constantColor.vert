#version 460 core

layout(location = 0) uniform mat4 uMatrix;
layout(location = 1) uniform mat4 uProjMatrix;

layout(location = 0) in vec4 vPosition;

void
main()
{
  gl_Position = uProjMatrix * uMatrix * vPosition;
}

