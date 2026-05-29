#version 330 core
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

uniform mat4 mvp;

out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 worldPos;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor    = vertexColor;
    worldPos     = vertexPosition;  // grass verts are submitted in world space via rlgl batch
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}
