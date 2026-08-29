#version 330 core

layout(location = 0) in vec2 a_position; // Quad vertex: [-1..1] or [0..1]
layout(location = 1) in vec2 a_texCoord; // UV coords: [0..1]

uniform mat4 u_projection;
uniform mat4 u_transform;

out vec2 v_texCoord;

void main() {
    v_texCoord = a_texCoord;
    gl_Position = u_projection * u_transform * vec4(a_position, 0.0, 1.0);
}
