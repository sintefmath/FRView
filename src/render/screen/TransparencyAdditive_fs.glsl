#version 330
#extension GL_ARB_shading_language_420pack : enable
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

layout(location=0)  out vec3            frag_color;
layout(binding=0)   uniform sampler2D   tex_solid_color;
layout(binding=1)   uniform sampler2D   tex_transparent_color;

void main(void)
{
    ivec2 coord = ivec2( gl_FragCoord.xy );
    vec3 solid_color = texelFetch( tex_solid_color, coord, 0 ).rgb;
    vec4 transparent_color = texelFetch( tex_transparent_color, coord, 0 );
    frag_color = transparent_color.rgb + max(0.0, (1.0-transparent_color.w))*solid_color;
}
