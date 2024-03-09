#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout(std140,binding = 0) uniform buffer{
	mat3 mvp;
	vec2 viewSize;
	vec2 dmy;
};

layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 tcoord;
layout (location = 0) out vec2 ftcoord;
layout (location = 1) out vec2 fpos;
void main(void) {
	ftcoord = tcoord;
	vec3 vv = mvp * vec3( 2.0*vertex.x/viewSize.x - 1.0, 2.0*vertex.y/viewSize.y - 1.0, 0 );
	fpos = vec2(vertex.x,vertex.y);
	gl_Position = vec4(vv.x, vv.y, 0, 1);
}