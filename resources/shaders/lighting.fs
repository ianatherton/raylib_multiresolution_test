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
uniform float useNormalMap; // 0 = geometry normal only; 1 = tangent-space texture1 (rocks)
uniform sampler2D texture0;
uniform sampler2D texture1; // tangent-space normal (OpenGL: Y+ up in map); MATERIAL_MAP_NORMAL

// Lighting parameters - using constants instead of uniforms for simplicity
const float ambientStrength = 0.2;
const float diffuseStrength = 1.0;
const float specularStrength = 0.5;
const float shininess = 16.0;

out vec4 fragColor;

void main()
{
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
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    fragColor = vec4(result, texColor.a);
}
