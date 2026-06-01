#version 330 core

in vec2 fragTexCoord;
in vec3 worldPos;

uniform sampler2D texture0;
uniform vec3 cameraPos;

// 4x4 Bayer ordered dither matrix
const float bayer[16] = float[16](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
);

#define DITHER_START  4.0
#define DITHER_END   45.0

#define MIP_START      4.0
#define MIP_TRANSITION 2.0

out vec4 finalColor;

void main()
{
    float dist     = length(worldPos - cameraPos);
    float mipBlend = clamp((dist - MIP_START) / MIP_TRANSITION, 0.0, 1.0);
    vec4 tex = mix(textureLod(texture0, fragTexCoord, 0.0), texture(texture0, fragTexCoord), mipBlend);

    if (tex.a < 0.5) discard;

    float factor = clamp((dist - DITHER_START) / (DITHER_END - DITHER_START), 0.1, 0.9);
    if (factor > 0.0) {
        ivec2 sc = ivec2(gl_FragCoord.xy) % 4;
        float threshold = bayer[sc.y * 4 + sc.x] / 16.0;
        if (factor > threshold) discard;
    }

    finalColor = tex;
}
