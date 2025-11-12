#version 460 core

uniform mat3 viewRotMatrix;
uniform mat4 projMatrix;

layout(location = 0) in vec3 vPosition;
out vec3 fPosition;

void
main()
{
     // Ici on prends pas en compte la translation de la vue
     vec3 vEyeCoord = viewRotMatrix * vPosition;
     gl_Position = projMatrix * vec4(vEyeCoord, 1.0);

     // Vertex position (world)
     // En effet, cette position peut servir pour calculer directement la direction de vue
     // ou les coordonn�e UV. Ici c'est parce que l'on utilise une sph�re
     fPosition = vPosition.xyz;
}

