#version 330 core 
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 UV;
in vec4 partColor;

uniform sampler2D myTex;

void main()
{
    vec4 color = texture(myTex, UV) * partColor;
    if(color.a < 0.1)
        discard;
    FragColor = color;

}
