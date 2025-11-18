#version 460 core

layout(binding = 0) uniform sampler2D texGrass;

in vec2 fUV;
out vec4 oColor;

void main()
{
    // Couleur finale
    vec4 color = texture(texGrass, fUV);
    if(color.a < 0.1)
        discard;
    
    oColor = color;
}