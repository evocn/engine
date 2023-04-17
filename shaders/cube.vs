#version 330 core

layout(location = 0) in vec3 VertexPosition;

out vec3 tex_coords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    // Cubemaps use vertex position for texturing.
    tex_coords = VertexPosition;
    // set position to be behind everything.
    vec4 pos = projection * view * vec4(VertexPosition, 1.0);
    gl_Position = pos.xyww;
}
