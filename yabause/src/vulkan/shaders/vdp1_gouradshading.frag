#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D u_sprite;
layout(location = 0) in vec4 v_texcoord;
layout(location = 1) in vec4 v_vtxcolor;
layout(location = 0) out vec4 fragColor;

void main() { 

  vec2 addr = v_texcoord.st;
  addr.s = addr.s / (v_texcoord.q);
  addr.t = addr.t / (v_texcoord.q);
  vec4 spriteColor = texture(u_sprite,addr);
  if( spriteColor.a == 0.0 ) discard; 
  fragColor = spriteColor;
  fragColor = clamp(spriteColor+v_vtxcolor,vec4(0.0),vec4(1.0));
  fragColor.a = spriteColor.a;
  
}

