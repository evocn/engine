#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec3 texCoords;
uniform samplerCube cubemap;

uniform float maskAmount;

void main()
{
    FragColor = mix(texture(cubemap, texCoords), vec4(0.0f, 0.0f, 0.0f, 1.0f), maskAmount);
}
