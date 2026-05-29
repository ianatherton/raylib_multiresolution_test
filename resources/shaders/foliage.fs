#version 330 core
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 worldPos;

uniform sampler2D texture0;
uniform vec3 cameraPos;

// 4x4 Bayer ordered dither matrix, values in [0, 15]
const float bayer[16] = float[16](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
);

#define DITHER_START  4.0
#define DITHER_END   45.0   // matches LOS_MAX_GRASS_DISTANCE

out vec4 finalColor;

void main()
{
    vec4 tex = texture(texture0, fragTexCoord);

    // Kill transparent pixels before they touch the depth buffer — fixes rectangular halos
    if (tex.a < 0.5) discard;

    // Screen-door LOD: discard up to 50% of pixels based on camera distance
    float dist   = length(worldPos - cameraPos);
    float factor = clamp((dist - DITHER_START) / (DITHER_END - DITHER_START), 0.0, 0.9375);
    if (factor > 0.0) {
        ivec2 sc = ivec2(gl_FragCoord.xy) % 4;
        float threshold = bayer[sc.y * 4 + sc.x] / 16.0;
        if (factor > threshold) discard;
    }

    finalColor = tex * fragColor;
}
