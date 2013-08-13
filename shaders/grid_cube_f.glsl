#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

layout(location=0)  out vec3                color;

in FI {
    vec2    obj_pos;
} in_f;

void main(void)
{



    vec2 t0 = 2.f*abs( fract( in_f.obj_pos ) - vec2(0.5f) );
    vec2 t1 = 2.f*abs( fract( 5f*in_f.obj_pos ) - vec2(0.5f) );

    vec2 w = fwidth( in_f.obj_pos );

    vec2 b0 = smoothstep( vec2(0.0), 3*w, t0 );
    vec2 b1 = smoothstep( vec2(0.0), 15*w, t1 );

    float a0 = clamp( 1.0-min(b0.x, b0.y), 0.0, 1.0 );
    float a1 = clamp( 0.5-min(b1.x, b1.y), 0.0, 1.0 );

    color = max(a0,a1)*vec3( 0.5, 0.8, 0.9 );
}
