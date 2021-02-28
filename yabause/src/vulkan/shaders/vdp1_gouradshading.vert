#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
   mat4 u_mvpMatrix;
   vec2 u_texsize;
} ubo;

layout (location = 0) in vec4 a_position;
layout (location = 1) in vec4 a_grcolor;
layout (location = 2) in vec4 a_texcoord;

layout(location = 0) out  vec4 v_texcoord;
layout(location = 1) out  vec4 v_vtxcolor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
   v_vtxcolor  = a_grcolor; 
   v_texcoord  = a_texcoord;
   v_texcoord.x  = v_texcoord.x / ubo.u_texsize.x;
   v_texcoord.y  = v_texcoord.y / ubo.u_texsize.y;
   gl_Position =  ubo.u_mvpMatrix * a_position;
}

