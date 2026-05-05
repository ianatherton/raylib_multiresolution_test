#version 330 core
// Vertex Shader for Blinn-Phong Lighting

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
in vec4 vertexTangent; // xyz = tangent, w = handedness for bitangent (MikkTSpace / raylib)

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;
out vec3 worldTangent;
out float tangentSign;

void main()
{
    fragPos = vec3(matModel * vec4(vertexPosition, 1.0));
    normal = mat3(matNormal) * vertexNormal;
    worldTangent = mat3(matNormal) * vertexTangent.xyz;
    tangentSign = vertexTangent.w;
    texCoord = vertexTexCoord;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
