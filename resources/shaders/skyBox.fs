#version 330 core
out vec4 FragColor;

in vec3 TexCoords;
in vec2 planetTexCoords;

uniform samplerCube skybox;
uniform sampler2D planetTexture;

void main()
{
    // Sample the skybox texture using the world position
        vec4 skyboxColor = texture(skybox, TexCoords);

        // Sample the planet texture using the additional texture coordinates
        vec4 planetColor = texture(planetTexture, planetTexCoords);

        // Mix the colors based on some condition or blending factor
        // Here, we can use the planet texture's alpha channel as a blending factor
        // Adjust this based on your requirements

        if(TexCoords.z>0.0){
            FragColor = skyboxColor;
        }
        else{
            FragColor = mix(skyboxColor, planetColor, planetColor.a);
        }
}