#version 450

layout(binding = 0) uniform vdp2regs { 
 mat4 u_mvpMatrix;   
 float u_pri[8]; 
 float u_alpha[8]; 
 vec4 u_coloroffset;
 float u_cctl;
 float u_emu_height;
 float u_vheight;
 int u_color_ram_offset;
 float u_viewport_offset;
 int u_sprite_window;
 float u_from;
 float u_to;
} ubo;

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 a_texcoord;

layout (location = 0) out vec4 v_texcoord;

void main() {
    v_texcoord  = a_texcoord;
    gl_Position = ubo.u_mvpMatrix * a_position;

}

