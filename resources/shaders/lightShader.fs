#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseTexture;

void main()
{
    vec4 color = texture(diffuseTexture, fs_in.TexCoords).rgba;
    if(color.a < 0.1)
            discard;
    //color.r *=4;
    //color.g *=4;
    //color.b *=4;
    float brightness = dot(color, vec4(0.2126, 0.7152, 0.0722, 1));
    if(brightness > 0.1)
        BrightColor = color;
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    //FragColor = vec4(color, 1.0);
    FragColor = color;
}
