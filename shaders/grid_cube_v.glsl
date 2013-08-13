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

uniform mat4 mvp;

out FI {
    vec2    obj_pos;
} out_v;

uniform mat4 unit_to_object;
uniform vec3 scale;

void main(void)
{
    vec4 oph = unit_to_object * vec4( position.xyz, 1.f );
    vec3 op = scale*(1.f/oph.w)*oph.xyz;
    if( position.w == 0.f ) {
        out_v.obj_pos = op.xy;
    }
    else if( position.w == 1.f ) {
        out_v.obj_pos = op.yz;
    }
    else {
        out_v.obj_pos = op.xz;
    }
//    out_v.obj_pos = (1.0/oph.w)*oph.xyz;
    gl_Position = mvp*vec4( position.xyz, 1.f );
}

