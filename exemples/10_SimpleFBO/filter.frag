#version 430 core
// Texture
layout(location = 0) uniform sampler2D iChannel0;
// If we want to do filtering or not
layout(location = 1) uniform bool useFilter;
// UV coordinates
in vec2 fUV;
// Out color
out vec4 fColor;

// Filter informations
// Adapted from: https://blog.maximeheckel.com/posts/on-crafting-painterly-shaders/
#define SECTOR_COUNT 8
layout(location = 2) uniform int radius;
layout(location = 3) uniform vec2 resolution;

vec3 sampleColor(vec2 offset) {
    vec2 coord = (gl_FragCoord.xy + offset) / resolution.xy;
    return abs(texture2D(iChannel0, coord).rgb);
}

void getSectorVarianceAndAverageColor(float angle, float radius, out vec3 avgColor, out float variance) {
    vec3 colorSum = vec3(0.0);
    vec3 squaredColorSum = vec3(0.0);
    float sampleCount = 0.0;

    for (float r = 1.0; r <= radius; r += 1.0) {
        for (float a = -0.392699; a <= 0.392699; a += 0.196349) {
            vec2 sampleOffset = r * vec2(cos(angle + a), sin(angle + a));
            vec3 color = sampleColor(sampleOffset);
            colorSum += color;
            squaredColorSum += color * color;
            sampleCount += 1.0;
        }
    }

    // Calculate average color and variance
    avgColor = colorSum / sampleCount;
    vec3 varianceRes = (squaredColorSum / sampleCount) - (avgColor * avgColor);
    variance = dot(varianceRes, vec3(0.299, 0.587, 0.114)); // Convert to luminance
}

void main()
{
    if(useFilter) {
        vec3 sectorAvgColors[SECTOR_COUNT];
        float sectorVariances[SECTOR_COUNT];

        for (int i = 0; i < SECTOR_COUNT; i++) {
            float angle = float(i) * 6.28318 / float(SECTOR_COUNT); // 2π / SECTOR_COUNT
            getSectorVarianceAndAverageColor(angle, float(radius), sectorAvgColors[i], sectorVariances[i]);
        }

        float minVariance = sectorVariances[0];
        vec3 finalColor = sectorAvgColors[0];

        for (int i = 1; i < SECTOR_COUNT; i++) {
            if (sectorVariances[i] < minVariance) {
                minVariance = sectorVariances[i];
                finalColor = sectorAvgColors[i];
            }
        }

        fColor = vec4(finalColor, 1.0);
    } else {
        // We use absolut value to better display the position
        fColor = abs(texture(iChannel0, fUV));
    }
}