#version 460 core

layout(location = 0) uniform mat4 mvMatrix;
layout (location = 1) uniform mat4 projMatrix;
layout (location = 2) uniform float size;
layout (location = 3) uniform float time;

// Wind texture 
layout(binding = 1) uniform sampler2D windTex;

// Conversion point to quads
layout (points) in;
layout (triangle_strip, max_vertices = 36) out;

// Input
in float vRandomOrientation[];

// Output with UV 
out vec2 fUV;

mat3 rotationMatrixY(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
        c, 0.0, -s,
        0.0, 1.0, 0.0,
        s, 0.0, c
    );
}
mat3 rotationX(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
        1.0, 0.0, 0.0,
        0.0, c, -s,
        0.0, s, c
    );
}
mat3 rotationZ(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
        c, -s, 0.0,
        s, c, 0.0,
        0.0, 0.0, 1.0
    );
}

void generateGrassQuad(vec4 pos, float halfSize, mat3 rotMat) {
    fUV = vec2(0.0, 0.0);
    gl_Position = projMatrix * mvMatrix *  (pos + vec4(rotMat * vec3(-halfSize, 0.0, 0.0), 0.0));
    EmitVertex();
    fUV = vec2(1.0, 0.0);
    gl_Position = projMatrix * mvMatrix * (pos + vec4(rotMat * vec3( halfSize, 0.0, 0.0), 0.0));
    EmitVertex();

    // Based on https://vulpinii.github.io/tutorials/grass-modelisation/en/
    vec2 windDirection = vec2(1.0, 1.0); 
    float windStrength = 0.1f;	
    // texture coordinates of the moving wind
    vec2 uv = pos.xz/10.0 + windDirection * windStrength * time ;
    uv.x = mod(uv.x,1.0); 
    uv.y = mod(uv.y,1.0);
    vec4 wind = texture(windTex, uv); 

    // Optional: remove gamma to have linear values 
    wind = pow(wind, vec4(2.2));

    mat3 modelWind =  (rotationX(wind.x*3.14159*0.75f - 3.14159*0.25f) * 
		  	rotationZ(wind.y*3.14159*0.75f - 3.14159*0.25f)); 
    if(time == 0.0){
        modelWind = mat3(1.0); // No wind at time 0 (disable wind)
    }

    // Top vertices with wind effect
    fUV = vec2(0.0, 1.0);
    gl_Position = projMatrix * mvMatrix * (pos + vec4(modelWind * rotMat * vec3(-halfSize, size, 0.0), 0.0));
    EmitVertex();
    fUV = vec2(1.0, 1.0);
    gl_Position = projMatrix * mvMatrix * (pos + vec4(modelWind * rotMat * vec3( halfSize, size, 0.0), 0.0));
    EmitVertex();
    EndPrimitive();
}

void main() {
    // Create a simple quad that will be always face the camera
    vec4 pos = gl_in[0].gl_Position;

    // Make it a vertical quad (y up) from the point position
    float halfSize = size * 0.5;

    // First quad
    mat3 rotMat = rotationMatrixY(vRandomOrientation[0]);
    generateGrassQuad(pos, halfSize, rotMat);

    // Second quad
    rotMat = rotationMatrixY(3.14159 / 4.0 + vRandomOrientation[0]);
    generateGrassQuad(pos, halfSize, rotMat);

    // Third quad
    rotMat = rotationMatrixY(-3.14159 / 4.0 + vRandomOrientation[0]);
    generateGrassQuad(pos, halfSize, rotMat);
}