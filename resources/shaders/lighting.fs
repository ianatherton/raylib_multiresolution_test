#version 330 core
// Fragment Shader for Blinn-Phong Lighting

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform sampler2D texture0;

// Lighting parameters - using constants instead of uniforms for simplicity
const float ambientStrength = 0.2;
const float diffuseStrength = 1.0;
const float specularStrength = 0.5;
const float shininess = 16.0;

out vec4 fragColor;

void main()
{
    // Ambient
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;

    // Specular
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // Texture color
    vec4 texColor = texture(texture0, texCoord);

    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    fragColor = vec4(result, texColor.a);
}
