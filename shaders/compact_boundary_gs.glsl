#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(points, invocations=1) in;
layout(points, max_vertices=1) out;

in VG {
    uvec2 info;
    uvec3 indices;
} in_gs[];

layout(location=0)  out uint        cell;
layout(location=1)  out uvec3       indices;
                    uniform bool    flip_faces;

void
main()
{
    uvec2 info = in_gs[0].info.xy;

    if( info.x == ~0u) {
        cell = info.y;
        indices = in_gs[0].indices.xyz;
        EmitVertex();
    }
    else if( info.y == ~0u ) {
        cell = info.x;
        indices = in_gs[0].indices.zyx;
        EmitVertex();
    }
}
