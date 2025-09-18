#version 460 core

// Entree de la couleur interpolee (vertex shader)
in vec4 ifColor;

// Tampon de couleur (image de sortie)
layout (location = 0) out vec4 fColor;

void
main()
{
    fColor = ifColor;
}