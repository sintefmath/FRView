#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

/** Main func for the surface compact paths */
layout(points, invocations=1) in;
layout(points, max_vertices=1) out;

in VG {
    uvec2 cell_ix;
    uvec3 vertex_ix;
    uvec3 normal_ix;
} in_gs[];

layout(location=0)  out uvec4       cell;
layout(location=1)  out uvec3       indices;
                    uniform bool    flip_faces;

bool
predicate( out uint cell, out bool flip, in uint cell_a, in uint cell_b );

void
main()
{
    uint selected_cell;
    bool flip;
    if( predicate( selected_cell, flip, in_gs[0].cell_ix.x, in_gs[0].cell_ix.y) ) {
        flip = flip ^^ flip_faces;
        if( !flip ) {
            cell    = uvec4( selected_cell, in_gs[0].normal_ix.xyz );
            indices = in_gs[0].vertex_ix;
        }
        else {
            //cell    = uvec4( selected_cell, in_gs[0].normal_ix.zyx );

            // Flip edge masks 0 and 1
            uint b01 =
                    ((selected_cell & (1u<<30u))>>1u) |
                    ((selected_cell & (1u<<29u))<<1u);
            selected_cell = (selected_cell & ~(3u<<29u)) | b01;

            cell    = uvec4( selected_cell, in_gs[0].normal_ix.z | 0x40000000u, in_gs[0].normal_ix.yx );
            indices = in_gs[0].vertex_ix.zyx;
        }
        EmitVertex();
    }
}
