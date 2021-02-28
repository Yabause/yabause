#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    vec4 u_color_offset;
};

layout(location = 0) in vec4 fragTexCoord;
layout(binding = 1) uniform sampler2D texSampler;
layout(location = 0) out vec4 outColor;

void main() {
    ivec2 addr;
    addr.x = int(fragTexCoord.x);
    addr.y = int(fragTexCoord.y);
    vec4 txcol = texelFetch( texSampler, addr,0 );
    if(txcol.a > 0.0){
        outColor = txcol + u_color_offset;
    }else{
        discard;
    }
}