#version 460

layout( triangles_adjacency ) in;
layout( triangle_strip, max_vertices = 18 ) out;

in vec3 fPosition[];

// In camera space
layout(location = 4) uniform vec3 lightPosition;
// projection matrix from the light
layout(location = 5) uniform mat4 projMatrix;

float EPSILON = 0.001;

// Check if one of direction have a different orientation
// compared to the normal of the triangle
bool facesLight( vec3 a, vec3 b, vec3 c )
{
    vec3 n = cross( b - a, c - a );
    vec3 center = (a + b + c) / 3.0;
    vec3 toLight = lightPosition - center;
    return dot(n, toLight) > 0;
}

// Create a infinite quad from a given edge
// The edge is extruded with the light direction 
void emitEdgeQuad( vec3 a, vec3 b ) {
    vec3 lightDirA = normalize(a - lightPosition);
    vec3 lightDirB = normalize(b - lightPosition);

    // Emit quad: a, a_extruded, b, b_extruded
    gl_Position = projMatrix * vec4(a + lightDirA * EPSILON, 1.0);
    EmitVertex();

    gl_Position = projMatrix * vec4(lightDirA, 0.0);  // At infinity
    EmitVertex();

    gl_Position = projMatrix * vec4(b + lightDirB * EPSILON, 1.0);
    EmitVertex();

    gl_Position = projMatrix * vec4(lightDirB, 0.0);  // At infinity
    EmitVertex();

    EndPrimitive();
}

void main()
{
    vec3 v0 = fPosition[0];
    vec3 v1 = fPosition[2];
    vec3 v2 = fPosition[4];

    if( facesLight(v0, v1, v2) ) {
        // Check adjacent triangles for silhouette edges
        if( !facesLight(fPosition[0], fPosition[1], fPosition[2]) )
            emitEdgeQuad(v1, v0);  // Reversed order for correct winding
        if( !facesLight(fPosition[2], fPosition[3], fPosition[4]) )
            emitEdgeQuad(v2, v1);
        if( !facesLight(fPosition[4], fPosition[5], fPosition[0]) )
            emitEdgeQuad(v0, v2);

        // Generation du front cap: 
        //  - On genere la surface juste en dessous de la surface
        //  - On utilise un epsilon pour mettre le cap avant
        vec3 lightDir0 = normalize(v0 - lightPosition);
        vec3 lightDir1 = normalize(v1 - lightPosition);
        vec3 lightDir2 = normalize(v2 - lightPosition);

        gl_Position = projMatrix * vec4(v0 + lightDir0 * EPSILON, 1.0);
        EmitVertex();
        gl_Position = projMatrix * vec4(v1 + lightDir1 * EPSILON, 1.0);
        EmitVertex();
        gl_Position = projMatrix * vec4(v2 + lightDir2 * EPSILON, 1.0);
        EmitVertex();
        EndPrimitive();

        // Generation du back: 
        //  - On prend la direction, et on la met a l'infini (w=0)
        gl_Position = projMatrix * vec4(lightDir0, 0.0);
        EmitVertex();
        gl_Position = projMatrix * vec4(lightDir2, 0.0); 
        EmitVertex();
        gl_Position = projMatrix * vec4(lightDir1, 0.0);
        EmitVertex();
        EndPrimitive();
    }
}