#version 460 core

layout(binding = 0) uniform sampler2D tex;
layout(location = 0) uniform float scale;

in vec2 fUV;

out vec4 oColor;

void main()
{
    oColor = texture(tex, fUV).rrrr * scale;
}
