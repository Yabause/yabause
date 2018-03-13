#ifndef __SCANLINE_INCLUDE_H__
#define __SCANLINE_INCLUDE_H__
static const GLchar Yglprg_blit_scanline_f[] =
"    vec3 _scanline;\n"
"    vec2 _x0009;\n"
"    float reduce;\n"
"    _x0009 = vTexCoord*vec2((3.1415*fWidth), 6.28299999*fHeight);\n"
"    reduce = 0.949999988 + (0.0500000007 * sin(_x0009.x) + 0.150000006 * sin(_x0009.y));\n"
"    _scanline = fragColor.xyz*reduce;\n"
"    fragColor = vec4(_scanline.x, _scanline.y, _scanline.z, 1.0);\n";

#endif
