#version 330 core
// Vertex Shader for Blinn-Phong Lighting

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;

void main()
{
    // World position of the vertex
    fragPos = vec3(matModel * vec4(vertexPosition, 1.0));
    // Normal in world space
    normal = mat3(matNormal) * vertexNormal;
    texCoord = vertexTexCoord;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
