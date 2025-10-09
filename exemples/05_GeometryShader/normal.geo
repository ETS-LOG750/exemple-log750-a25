#version 460 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout(location = 2) uniform float uScale;
layout(location = 3) uniform bool showCenter;

in vec3 fNormal[];

// Example of generating lines per vertices
void GenerateLine(int index)
{
    // Right to left handed
    // All the primitives needs to be properly generated
    vec4 projection = vec4(1, 1, -1, 1);

    gl_Position = projection * gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = projection * (gl_in[index].gl_Position + 
                                vec4(fNormal[index], 0.0) * uScale);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    if(!showCenter) {
        GenerateLine(0); // first vertex normal
        GenerateLine(1); // second vertex normal
        GenerateLine(2); // third vertex normal
    } else {
        // Compute the average position (middle face)
        vec4 c = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) / 3;
        // Compute the average normal 
        vec3 n = (fNormal[0] + fNormal[1] + fNormal[2]) / 3;

        // Emit the line geometry
        vec4 projection = vec4(1, 1, -1, 1);
        gl_Position = projection * c;
        EmitVertex();
        gl_Position = projection * (c + vec4(n, 0) * uScale);
        EmitVertex();
        EndPrimitive();
        
    }
}  