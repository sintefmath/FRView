#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0) in vec2  spos;
layout(location=1) in vec2  tpos;

uniform mat4                mvp;
uniform vec3                position;
uniform vec2                size;
uniform vec2                scale;

out FS {
    vec2                    tpos;
}                           out_fs;

void main(void)
{
    vec4 h = mvp * vec4( position, 1.f );
    vec3 p = (1.f/h.w)*h.xyz;
    vec2 sp = vec2( ivec2(size*p.xy) + vec2( 0.5f ));

    out_fs.tpos = tpos;
    gl_Position = vec4( scale*( sp.xy + spos),
                        p.z,
                        1.f);

}
