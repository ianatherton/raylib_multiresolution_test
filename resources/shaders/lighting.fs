#version 330 core
// Fragment Shader for Blinn-Phong Lighting

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;
in vec3 worldTangent;
in float tangentSign;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec2 uvScale; // (1,1) scene; higher = more tiling on props when set before draw
uniform float useNormalMap;   // 0 = geometry normal only; 1 = sample texture1 for TBN normals
uniform float useMetalRough;  // 0 = constant spec; 1 = texture2 metallic + texture3 roughness
uniform float useParallax;    // 0 = off; 1 = parallax offset UVs via texture3 height map
uniform sampler2D texture0; // albedo
uniform sampler2D texture1; // normal map
uniform sampler2D texture2; // detail normal map (terrain) / metallic (char)
uniform sampler2D texture3; // roughness (char) / height map (terrain)
uniform float useDetailNormal; // 1 = blend texture2 as detail normal map

const float ambientStrength  = 0.2;
const float diffuseStrength  = 1.0;
const float specularStrength = 0.01;
const float shininess        = 16.0;
uniform float parallaxScale;

#define MIP_START           4.0
#define MIP_TRANSITION      2.0
#define DETAIL_NORMAL_TILE  7.0  // detail normal repeats 7x more than the base texture

#define TSAMPLE(tex, uv, blend) mix(textureLod(tex, uv, 0.0), texture(tex, uv), blend)

out vec4 fragColor;

void main()
{
    float dist     = length(fragPos - viewPos);
    float mipBlend = clamp((dist - MIP_START) / MIP_TRANSITION, 0.0, 1.0);

    vec2 tiledUV = texCoord * uvScale;

    if (useParallax > 0.5) {
        vec3 Ngeom = normalize(normal);
        vec3 T = normalize(worldTangent - dot(worldTangent, Ngeom) * Ngeom);
        vec3 B = normalize(cross(Ngeom, T) * tangentSign);
        vec3 viewDirTS = normalize(transpose(mat3(T, B, Ngeom)) * normalize(viewPos - fragPos));
        float h = TSAMPLE(texture3, tiledUV, mipBlend).r;
        tiledUV += (viewDirTS.xy / max(viewDirTS.z, 0.1)) * (h - 0.5) * parallaxScale;
    }

    vec4 texColor = TSAMPLE(texture0, tiledUV, mipBlend);

    vec3 Ngeom = normalize(normal);
    vec3 N = Ngeom;
    if (useNormalMap > 0.5 || useDetailNormal > 0.5) {
        vec3 tIn = worldTangent;
        vec3 T = normalize(tIn - dot(tIn, Ngeom) * Ngeom);
        vec3 B = normalize(cross(Ngeom, T) * tangentSign);
        mat3 TBN = mat3(T, B, Ngeom);

        vec3 tsN = (useNormalMap > 0.5)
            ? TSAMPLE(texture1, tiledUV, mipBlend).rgb * 2.0 - 1.0
            : vec3(0.0, 0.0, 1.0);

        if (useDetailNormal > 0.5) {
            vec2 detailUV = tiledUV * DETAIL_NORMAL_TILE;
            vec3 d = TSAMPLE(texture2, detailUV, mipBlend).rgb * 2.0 - 1.0;
            d.xy *= 1.3;
            d = normalize(d);
            tsN = normalize(vec3(tsN.xy + d.xy, tsN.z));
        }

        N = normalize(TBN * tsN);
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
        float metallic  = TSAMPLE(texture2, tiledUV, mipBlend).r;
        float roughness = TSAMPLE(texture3, tiledUV, mipBlend).r;
        specStr = mix(0.02, 0.9, metallic);
        shine   = mix(4.0, 128.0, 1.0 - roughness);
    }
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shine);
    vec3 specular = specStr * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    fragColor = vec4(result, texColor.a);
}
