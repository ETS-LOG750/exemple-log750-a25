#version 460 core

layout(location = 0) in  vec4  vPosition;

void
main()
{
    // No transform -- the transform will be done inside the tesselation shader
    gl_Position = vPosition;
}
