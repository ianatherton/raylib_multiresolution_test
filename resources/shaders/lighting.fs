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
uniform sampler2D texture2; // metallic / roughness (char)
uniform sampler2D texture3; // roughness (char) / height map (terrain)

const float ambientStrength  = 0.2;
const float diffuseStrength  = 1.0;
const float specularStrength = 0.01;
const float shininess        = 16.0;
uniform float parallaxScale;

out vec4 fragColor;

void main()
{
    vec2 tiledUV = texCoord * uvScale;

    if (useParallax > 0.5) {
        vec3 Ngeom = normalize(normal);
        vec3 T = normalize(worldTangent - dot(worldTangent, Ngeom) * Ngeom);
        vec3 B = normalize(cross(Ngeom, T) * tangentSign);
        vec3 viewDirTS = normalize(transpose(mat3(T, B, Ngeom)) * normalize(viewPos - fragPos));
        float h = texture(texture3, tiledUV).r;
        tiledUV += (viewDirTS.xy / max(viewDirTS.z, 0.1)) * (h - 0.5) * parallaxScale;
    }

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
