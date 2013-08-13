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
layout(binding=0)   uniform usamplerBuffer  cell_subset;


bool
selected( uint cell )
{
    cell = cell & 0x0fffffffu;
    uint s = ((texelFetch( cell_subset, int(cell/32u) ).r)>>(cell%32u))&0x1u;
    return s == 1u;
}


void
main()
{
    uvec2 info;
    if( flip_faces ) {
        info = in_gs[0].info.yx;
    }
    else {
        info = in_gs[0].info.xy;
    }


    uint cell_a = info.x;
    uint cell_b = info.y;

    if( cell_a == ~0u ) {
        if( !selected( cell_b ) ) {
            cell    = info.y;
            indices = in_gs[0].indices;
            EmitVertex();
        }
    }
    else if( cell_b == ~0u ) {
        if( !selected( cell_a ) ) {
            cell    = info.x;
            indices = in_gs[0].indices.zyx;
            EmitVertex();
        }
    }
}
