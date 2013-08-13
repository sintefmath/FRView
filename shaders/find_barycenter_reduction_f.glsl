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

layout(pixel_center_integer) in vec4    gl_FragCoord;

                    uniform int         level;
layout(binding=0)   uniform sampler2D   texture_barycenter;
layout(location=0)  out vec3            buffer_barycenter;

void main(void)
{
    buffer_barycenter = texelFetch( texture_barycenter, ivec2(2*gl_FragCoord.xy + vec2(0,0)), level ).rgb +
                        texelFetch( texture_barycenter, ivec2(2*gl_FragCoord.xy + vec2(0,1)), level ).rgb +
                        texelFetch( texture_barycenter, ivec2(2*gl_FragCoord.xy + vec2(1,0)), level ).rgb +
                        texelFetch( texture_barycenter, ivec2(2*gl_FragCoord.xy + vec2(1,1)), level ).rgb;
}
