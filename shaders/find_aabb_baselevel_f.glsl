#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

in FI {
    flat vec3 position;
} in_f;
                    uniform mat4            inv_transform;
layout(location=0)  out vec3                buffer_min;
layout(location=1)  out vec3                buffer_max;

void main(void)
{
    vec4 ph = inv_transform * vec4( in_f.position, 1.f );
    vec3 p = (1.f/ph.w)*ph.xyz;
    buffer_min = p;
    buffer_max = p;
}
