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

layout(pixel_center_integer)    in vec4             gl_FragCoord;
                                uniform int         level;
layout(binding=0)               uniform sampler2D   texture_cov0;
layout(binding=1)               uniform sampler2D   texture_cov1;
layout(location=0)              out vec3            buffer_cov0;
layout(location=1)              out vec3            buffer_cov1;

void main(void)
{
    buffer_cov0 = texelFetch( texture_cov0, ivec2(2*gl_FragCoord.xy + vec2(0,0)), level ).rgb +
                  texelFetch( texture_cov0, ivec2(2*gl_FragCoord.xy + vec2(0,1)), level ).rgb +
                  texelFetch( texture_cov0, ivec2(2*gl_FragCoord.xy + vec2(1,0)), level ).rgb +
                  texelFetch( texture_cov0, ivec2(2*gl_FragCoord.xy + vec2(1,1)), level ).rgb;
    buffer_cov1 = texelFetch( texture_cov1, ivec2(2*gl_FragCoord.xy + vec2(0,0)), level ).rgb +
                  texelFetch( texture_cov1, ivec2(2*gl_FragCoord.xy + vec2(0,1)), level ).rgb +
                  texelFetch( texture_cov1, ivec2(2*gl_FragCoord.xy + vec2(1,0)), level ).rgb +
                  texelFetch( texture_cov1, ivec2(2*gl_FragCoord.xy + vec2(1,1)), level ).rgb;
}
