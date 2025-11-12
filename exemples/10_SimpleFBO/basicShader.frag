#version 430 core

layout(location = 3) uniform vec3 Kd;
layout(location = 4) uniform vec3 Ks;
layout(location = 5) uniform float Kn;
layout(location = 6) uniform vec3 lightPos;
layout(location = 7) uniform vec3 lightPos2;
layout(location = 8) uniform vec3 lightPos3;


in vec3 fNormal;
in vec3 fPosition;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec3 oPosition;

vec3 compute_direct_point(vec3 normal, vec3 viewDir, vec3 position, vec3 lightPosition, vec3 intensity) {
   
    // Compute light direction
    float dist = length(lightPosition-fPosition);
    vec3 LightDirection = (lightPosition-fPosition)/dist;

  
    // Custom attenuation (less agressive than 1/(d^2))
    vec3 I_light = intensity/(1.0+0.1*dist+0.01*dist*dist);

    // Compute diffuse component
    vec3 diffuse = Kd * max(0.0, dot(normal, LightDirection)) * I_light;

    // Compute specular component
    vec3 Rl = normalize(-LightDirection+2.0*normal*dot(normal,LightDirection));
    vec3 specular = Ks*pow(max(0.0, dot(Rl, viewDir)), Kn) * I_light;

    return diffuse +  specular;
}

void main()
{
    // Get lighting vectors
    vec3 nfNormal = normalize(fNormal);
    vec3 nviewDirection = normalize(vec3(0.0)-fPosition);

    vec3 contribLight1 = compute_direct_point(nfNormal, nviewDirection, fPosition, lightPos, vec3(1.0, 1.0, 1.0));
    vec3 contribLight2 = compute_direct_point(nfNormal, nviewDirection, fPosition, lightPos2, vec3(0.8, 0.1, 0.1));
    vec3 contribLight3 = compute_direct_point(nfNormal, nviewDirection, fPosition, lightPos3, vec3(0.1, 0.1, 0.8));

    // Compute final color
    oColor = vec4(contribLight1 + contribLight2 + contribLight3, 1);
    oPosition = fPosition;
}
