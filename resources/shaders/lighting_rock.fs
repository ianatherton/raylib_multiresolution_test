#version 330 core
// Blinn-Phong lighting with Bayer screen-door LOD for rock props

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;
in vec3 worldTangent;
in float tangentSign;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec2 uvScale;
uniform float useNormalMap;
uniform float useMetalRough;
uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;

const float ambientStrength  = 0.2;
const float diffuseStrength  = 1.0;
const float specularStrength = 0.01;
const float shininess        = 16.0;

const float bayer[16] = float[16](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
);

#define ROCK_DITHER_START 15.0
#define ROCK_DITHER_END   80.0  // matches LOS_MAX_ROCK_DISTANCE

out vec4 fragColor;

void main()
{
    float dist   = length(fragPos - viewPos);
    float factor = clamp((dist - ROCK_DITHER_START) / (ROCK_DITHER_END - ROCK_DITHER_START), 0.0, 0.8);
    if (factor > 0.0) {
        ivec2 sc = ivec2(gl_FragCoord.xy) % 4;
        float threshold = bayer[sc.y * 4 + sc.x] / 16.0;
        if (factor > threshold) discard;
    }

    vec2 tiledUV = texCoord * uvScale;
    vec4 texColor = texture(texture0, tiledUV);

    vec3 Ngeom = normalize(normal);
    vec3 N = Ngeom;
    if (useNormalMap > 0.5) {
        vec3 tIn = worldTangent;
        vec3 T = normalize(tIn - dot(tIn, Ngeom) * Ngeom);
        vec3 B = normalize(cross(Ngeom, T) * tangentSign);
        mat3 TBN = mat3(T, B, Ngeom);
        vec3 mapN = texture(texture1, tiledUV).rgb * 2.0 - 1.0;
        N = normalize(TBN * mapN);
    }

    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, N);

    float specStr = specularStrength;
    float shine = shininess;
    if (useMetalRough > 0.5) {
        float metallic  = texture(texture2, tiledUV).r;
        float roughness = texture(texture3, tiledUV).r;
        specStr = mix(0.02, 0.9, metallic);
        shine   = mix(4.0, 128.0, 1.0 - roughness);
    }
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shine);
    vec3 specular = specStr * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    fragColor = vec4(result, texColor.a);
}
