#version 430 core

in vec3 fColor;

out vec4 oColor;

void
main()
{
    // La rasterisation effectue l'interpolation des couleurs des sommets
    // Ces couleurs sont calculées dans le vertex shader
    oColor = vec4(fColor, 1.0);
}
