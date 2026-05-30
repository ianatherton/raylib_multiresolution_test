#version 330 core

// Base quad vertex (local space: x in [-0.5, 0.5], y in [0, height])
layout(location = 0) in vec2 quadPos;
layout(location = 1) in vec2 texCoord;

// Per-instance (divisor = 1)
layout(location = 2) in vec3  instancePos;
layout(location = 3) in float instanceYaw;
layout(location = 4) in float instancePitch;
layout(location = 5) in float instanceSpeed;
layout(location = 6) in float instancePhase;
layout(location = 7) in float instanceMaxLean;

uniform mat4 mvp;
uniform float time;

out vec2 fragTexCoord;
out vec3 worldPos;

vec3 rotateY(vec3 v, float a) {
    float s = sin(a), c = cos(a);
    return vec3(c*v.x + s*v.z, v.y, -s*v.x + c*v.z);
}
vec3 rotateX(vec3 v, float a) {
    float s = sin(a), c = cos(a);
    return vec3(v.x, c*v.y - s*v.z, s*v.y + c*v.z);
}
vec3 rotateZ(vec3 v, float a) {
    float s = sin(a), c = cos(a);
    return vec3(c*v.x - s*v.y, s*v.x + c*v.y, v.z);
}

void main()
{
    float leanAx = sin(time * instanceSpeed + instancePhase) * instanceMaxLean;
    float leanAz = cos(time * instanceSpeed * 0.73 + instancePhase * 1.37) * instanceMaxLean * 0.48;

    // Scale quad to grass dimensions (w=1.0, h=1.5 baked into base quad)
    vec3 pos = vec3(quadPos.x, quadPos.y, 0.0);
    // Spatial orientation (same order as original CPU transform)
    pos = rotateY(pos, instanceYaw);
    pos = rotateX(pos, instancePitch);
    // Wind lean applied on top
    pos = rotateX(pos, leanAx);
    pos = rotateZ(pos, leanAz);

    worldPos = instancePos + pos;
    fragTexCoord = texCoord;
    gl_Position = mvp * vec4(worldPos, 1.0);
}
