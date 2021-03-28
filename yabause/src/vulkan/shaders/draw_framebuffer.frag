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


layout(binding = 1) uniform sampler2D s_vdp1FrameBuffer;
layout(binding = 2) uniform sampler2D s_vdp1Cram;
layout(binding = 3) uniform sampler2D s_line;

layout(location = 0) in vec4 v_texcoord;
layout(location = 0) out vec4 fragColor;

void main()
{
  
  vec4 addr = v_texcoord;
  vec4 fbColor = texture(s_vdp1FrameBuffer,vec2(addr.x,addr.y));
  int additional = int(fbColor.a * 255.0);
  highp float alpha = float((additional/8)*8)/255.0;
  highp float depth = (float(additional&0x07)/10.0) + 0.05;
  if( depth < ubo.u_from || depth > ubo.u_to ){
    discard;
  }else if( alpha > 0.0){
     fragColor = fbColor;
     fragColor += ubo.u_coloroffset; 
     fragColor.a = alpha + 7.0/255.0;
     gl_FragDepth = (depth+1.0)/2.0;
  }else{
     discard;
  }
  
}

