#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;
out vec2 planetTexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    planetTexCoords = vec2(TexCoords.x > 0.5 ? (TexCoords.x - 0.5) * 2.0 : 0.0,
                                TexCoords.y > 0.5 ? (TexCoords.y - 0.5) * 2.0: 0.0);

    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}