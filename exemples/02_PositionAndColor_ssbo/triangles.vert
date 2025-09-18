#version 460 core

// Entree du nuanceurs de sommets
// Note: performance could be improved if the struct was memory aligned
struct Vertex {
     float position[3];
     float color[4];
};
layout(binding = 0, std430) buffer ssbo1 {
    Vertex vertices[];
};


// Sortie: couleur sera interpolee et passee dans le nuanceur de fragment
out vec4 ifColor;

void main()
{
     Vertex v = vertices[gl_VertexID];
     gl_Position = vec4(v.position[0], v.position[1], v.position[2], 1);
     ifColor = vec4(v.color[0], v.color[1], v.color[2], v.color[3]);
}

