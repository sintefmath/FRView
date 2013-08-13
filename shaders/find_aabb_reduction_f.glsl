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

                    uniform int         level;
layout(binding=0)   uniform sampler2D   texture_min;
layout(binding=1)   uniform sampler2D   texture_max;
layout(location=0)  out vec3            buffer_min;
layout(location=1)  out vec3            buffer_max;

void main(void)
{
    buffer_min = min( texelFetch( texture_min, 2*int(gl_FragCoord.xy) + ivec2(0,0), level ).rgb,
                      min( texelFetch( texture_min, 2*int(gl_FragCoord.xy) + ivec2(0,1), level ).rgb,
                           min( texelFetch( texture_min, 2*int(gl_FragCoord.xy) + ivec2(1,0), level ).rgb,
                                texelFetch( texture_min, 2*int(gl_FragCoord.xy) + ivec2(1,1), level ).rgb ) ) );
    buffer_max = max( texelFetch( texture_max, 2*int(gl_FragCoord.xy) + ivec2(0,0), level ).rgb,
                      max( texelFetch( texture_max, 2*int(gl_FragCoord.xy) + ivec2(0,1), level ).rgb,
                           max( texelFetch( texture_max, 2*int(gl_FragCoord.xy) + ivec2(1,0), level ).rgb,
                                texelFetch( texture_max, 2*int(gl_FragCoord.xy) + ivec2(1,1), level ).rgb ) ) );
}
