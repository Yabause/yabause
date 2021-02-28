#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    vec4 u_color_offset;
};


layout(location = 0) in vec4 v_texcoord;
layout(binding = 1) uniform sampler2D s_texture;
layout(binding = 2) uniform sampler2D s_color;
layout(location = 0) out vec4 fragColor;

void main() {
    vec4 txindex = texelFetch( s_texture, ivec2(int(v_texcoord.x),int(v_texcoord.y)) ,0 );
    if(txindex.a == 0.0) { discard; }
    vec4 txcol = texelFetch( s_color,  ivec2( ( int(txindex.g*65280.0) | int(txindex.r*255.0)) ,0 )  , 0 );
    fragColor = txcol + u_color_offset;
    fragColor.a = txindex.a;
}