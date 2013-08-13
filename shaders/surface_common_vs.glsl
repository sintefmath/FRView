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

out GI {
    vec3    obj_pos;
    vec2    scr_pos;
} in_g;

uniform mat4            MVP;
uniform mat3            NM;
uniform vec2            screen_size;

void main(void)
{
    gl_Position = MVP*position;
    in_g.obj_pos = position.xyz;
    in_g.scr_pos = screen_size*( (0.5f/gl_Position.w)*gl_Position.xy + vec2(0.5f) );
}
