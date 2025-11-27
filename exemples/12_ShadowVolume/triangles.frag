#version 460

layout(location = 3) uniform vec3 lightPositionCameraSpace;
layout(location = 4) uniform vec4 uColor;

in vec3 fNormal;
in vec3 fPosition;

out vec4 oColor;

void main()
{
    vec3 LightDirection = normalize(lightPositionCameraSpace-fPosition);
    float diffuse = max(0.0, dot(fNormal, LightDirection));

    // Diffuse and ambient lighting.
    vec4 materialColor = uColor;
    oColor = materialColor * diffuse;
}
