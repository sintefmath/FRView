#version 330
#extension GL_ARB_shading_language_420pack : enable
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

layout(location=0)  out vec3            frag_color;
layout(binding=0)   uniform sampler2D   tex_solid_color;
layout(binding=1)   uniform sampler2D   tex_transparent_color;
layout(binding=2)   uniform sampler2D   tex_transparent_complexity;


void main(void)
{
    ivec2 coord = ivec2( gl_FragCoord.xy );

    vec3 color = texelFetch( tex_solid_color, coord, 0 ).rgb;
    float complexity = texelFetch( tex_transparent_complexity, coord, 0 ).r;
    if( complexity > 0.f ) {
        vec4 transparent_color = texelFetch( tex_transparent_color, coord, 0 );

        float rcp = transparent_color.w;
        if( rcp > 0.001f ) {
            rcp = 1.f/rcp;
        }
        vec3 avg_color = rcp*transparent_color.rgb;
        float avg_alpha = transparent_color.w/complexity;
        float w0 = pow( 1.f-avg_alpha, complexity );
        float w1 = 1.f-w0;
        color = w1*avg_color + w0*color;
    }
    frag_color = color;
}
