#version 330 core
in vec2 fragTexCoord;
out vec4 fragColor;
uniform sampler2D sharpTex;
uniform sampler2D blurTex;
uniform sampler2D depthScene;
uniform sampler2D depthProps;
uniform sampler2D propsColorTex;
uniform mat4 invViewProj; // inverse(view * proj), same basis as raylib Vector3Unproject
uniform vec3 camPos;
uniform float dofSharpRadiusM; // no blur within this world distance from camera
uniform float dofBlurFullDistM;

void main()
{
    vec2 uv = fragTexCoord;
    vec4 sharp = texture(sharpTex, uv);
    vec4 blurCol = texture(blurTex, uv);
    float ds = texture(depthScene, uv).r;
    float dp = texture(depthProps, uv).r;
    vec4 pc = texture(propsColorTex, uv);
    float d = mix(ds, dp, pc.a);
    vec2 ndc = vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    vec4 w = invViewProj * vec4(ndc.x, ndc.y, d, 1.0);
    vec3 worldPos = w.xyz / w.w;
    float dist = length(worldPos - camPos);
    float blurAmt = smoothstep(dofSharpRadiusM, dofBlurFullDistM, dist);
    fragColor = vec4(mix(sharp.rgb, blurCol.rgb, blurAmt), sharp.a);
}
