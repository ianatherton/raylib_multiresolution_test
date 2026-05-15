#version 330 core
in vec2 fragTexCoord;
out vec4 fragColor;
uniform sampler2D image;
uniform vec2 texelDir; // (1/w,0) or (0,1/h) times optional strength

void main()
{
    vec2 uv = fragTexCoord;
    vec3 c = texture(image, uv).rgb * 0.2270270270;
    c += texture(image, uv + texelDir * 1.0).rgb * 0.1945945946;
    c += texture(image, uv - texelDir * 1.0).rgb * 0.1945945946;
    c += texture(image, uv + texelDir * 2.0).rgb * 0.1216216216;
    c += texture(image, uv - texelDir * 2.0).rgb * 0.1216216216;
    c += texture(image, uv + texelDir * 3.0).rgb * 0.0540540541;
    c += texture(image, uv - texelDir * 3.0).rgb * 0.0540540541;
    c += texture(image, uv + texelDir * 4.0).rgb * 0.0162162162;
    c += texture(image, uv - texelDir * 4.0).rgb * 0.0162162162;
    fragColor = vec4(c, 1.0);
}
