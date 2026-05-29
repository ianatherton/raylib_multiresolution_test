#version 330 core
// Instanced version of lighting.vs — mvp is VP only; model comes from instanceTransform attribute.

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;
layout(location = 4) in vec4 vertexTangent;
layout(location = 9) in mat4 instanceTransform;  // occupies locations 9, 10, 11, 12

uniform mat4 mvp;  // view-projection only (model = instanceTransform)

out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;
out vec3 worldTangent;
out float tangentSign;

void main()
{
    fragPos = vec3(instanceTransform * vec4(vertexPosition, 1.0));
    mat3 normalMat = transpose(inverse(mat3(instanceTransform)));
    normal = normalMat * vertexNormal;
    worldTangent = normalMat * vertexTangent.xyz;
    tangentSign = vertexTangent.w;
    texCoord = vertexTexCoord;
    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
}
