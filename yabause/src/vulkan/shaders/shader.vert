#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    vec4 u_color_offset;
};

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec4 a_texcoord;

layout(location = 0) out vec4 fragTexCoord;

void main() {
    gl_Position = mvp * a_position;
    fragTexCoord = a_texcoord;     
}


