#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0) in vec3 position;
layout(location=1) in vec3 color;

out FI {
    vec3    color;
} in_f;

uniform mat4 mv;

void
main()
{
    in_f.color = color;
    gl_Position = mv * vec4( position, 1.f );
}
