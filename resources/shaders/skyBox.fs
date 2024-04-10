#version 330 core
out vec4 FragColor;

in vec3 TexCoords;
in vec2 planetTexCoords;

uniform samplerCube skybox;
uniform sampler2D planetTexture;

void main()
{
        vec4 skyboxColor = texture(skybox, TexCoords);
        vec4 planetColor = texture(planetTexture, planetTexCoords);

        if(TexCoords.z>0.0){
            FragColor = skyboxColor;
        }
        else{
            FragColor = mix(skyboxColor, planetColor, planetColor.a);
        }
}