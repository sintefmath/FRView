#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0) in vec4 position;

out FI {
    flat vec3 position;
} in_f;

uniform int     size_l2;
uniform int     size_mask;
uniform float   scale;

void main(void)
{
    float x = scale*(gl_VertexID & size_mask) - 1.f;
    float y = scale*((gl_VertexID >> size_l2)& size_mask) - 1.f;

    in_f.position = position.xyz;
    gl_Position = vec4( x, y, 0.f, 1.f );
}

