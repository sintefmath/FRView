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

layout(location=0)  out vec4            frag_color;
layout(binding=0)   uniform sampler2D   transparent_color;
layout(binding=1)   uniform sampler2D   transparent_complexity;

void main(void)
{
    ivec2 coord = ivec2( gl_FragCoord.xy );
    float complexity = texelFetch( transparent_complexity, coord, 0 ).r;
    if( complexity < 1.f ) {
        discard;
    }

    vec4 transparent = texelFetch( transparent_color, coord, 0 );
    float acc = transparent.w;
    if( acc > 0.001f ) {
        acc = 1.f/acc;
    }
    float avg_alpha = transparent.w/complexity;
    frag_color = vec4( acc*transparent.rgb, avg_alpha );
}
