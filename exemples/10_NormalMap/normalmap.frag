#version 400 core
uniform sampler2D texColor;
uniform sampler2D texNormal;
uniform sampler2D texARM;

uniform bool activateARM;
uniform bool activateNormalMap;
uniform vec3 lightDirection;

in vec2 fUV;
in vec3 fNormal;
in vec3 fTangent;
in vec3 fBitangent;
in vec3 fPosition;


// Out shading color
out vec4 fColor;

void
main()
{
    // Build the matrix to transform from XYZ (normal map) space to TBN (tangent) space
    // Each vector fills a column of the matrix
    vec3 normal;
    if(activateNormalMap) {
        mat3 tbn = mat3(normalize(fTangent), normalize(fBitangent), normalize(fNormal));
        vec3 normalFromTexture = texture(texNormal, fUV).rgb * 2.0 - vec3(1.0);
        normal = normalize(tbn * normalFromTexture);
    } else {
        normal = normalize(fNormal);
    }
    
    // As position is expressed in camera space
    // meaning that the camera position is at the origin (0,0,0)
    vec3 nViewDirection = normalize(-fPosition); 

    // Read AO, Roughness and Metallic texture
    float ao = 1.0; // No ambient occlusion by default
    float n = 32; // Default Phong exponent
    float propSpec = 0.3; // Default specular proportion
    if(!activateARM) {
        // Default values
        vec3 ARM = texture(texARM, fUV).rgb;
        ao = ARM.r;
        
        // Convert roughness to phong exponent
        float roughness = ARM.g;
        float alpha = roughness * roughness;
        n = mix(4.0, 256.0, 1.0 - alpha); // Phong exponent from 4 to 256

        float metallic = ARM.b;
        propSpec = 0.04 + (1.0 - 0.04) * metallic; // Make sure to have some specular
    }    

    // Shading
    vec3 intensity = vec3(1.0); // White light
    float cosTheta = max(0.0, dot(normal, lightDirection));
    vec3 finalColor;
    if(cosTheta != 0.0) {
        vec3 Rl = normalize(-lightDirection+2.0*normal*cosTheta);
        float specular = pow(max(0.0, dot(Rl, nViewDirection)), n);
        vec3 baseColor = texture(texColor, fUV).rgb;
        // Applied the ambient occlusion only to the diffuse part
        // Kd = Ks uses the base color
        finalColor = (ao + intensity) * baseColor * cosTheta * (1-propSpec) + baseColor * specular * propSpec * intensity;
    } else {
        finalColor = vec3(0.0);
    }
    fColor = vec4(finalColor, 1.0);

    // Gamma correction (if necessary)
    // fColor = clamp(fColor, 0.0, 1.0);
    // fColor = pow(fColor, vec4(1.0/2.2));
}
