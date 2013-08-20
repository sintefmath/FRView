#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

layout(points, invocations=1) in;
layout(triangle_strip, max_vertices=4) out;
in GI {
    vec3 bbmin;
    vec3 bbmax;
    uint cell;
} gi[];

out FI {
    flat uint cell;
} go;

uniform vec2            slice;

void
main()
{
    if( (slice.x <= gi[0].bbmax.z ) && (gi[0].bbmin.z <= slice.y)  ) {
        go.cell = gi[0].cell;
        gl_Position = vec4( gi[0].bbmin.x, gi[0].bbmin.y, 0.f, 1.f );
        EmitVertex();
        gl_Position = vec4( gi[0].bbmax.x, gi[0].bbmin.y, 0.f, 1.f );
        EmitVertex();
        gl_Position = vec4( gi[0].bbmin.x, gi[0].bbmax.y, 0.f, 1.f );
        EmitVertex();
        gl_Position = vec4( gi[0].bbmax.x, gi[0].bbmax.y, 0.f, 1.f );
        EmitVertex();
    }
}
