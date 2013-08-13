#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0)  out vec4 frag_color;

in FI {
    vec3    color;
} in_f;

void
main()
{
    frag_color = vec4( in_f.color, 1.f );
}
