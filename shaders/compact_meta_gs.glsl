#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

/** Main func for the surface compact paths */
layout(points, invocations=1) in;
layout(points, max_vertices=1) out;

in VG {
    uvec2 cell_ix;
    uint  offset_a;
    uint  offset_b;
} in_gs[];

layout(location=0,stream=0) out uvec4               meta_subset;
layout(location=0,stream=1) out uvec4               meta_boundary;
layout(location=0,stream=2) out uvec4               meta_fault;
layout(binding=0)           uniform usamplerBuffer  cell_subset;
                            uniform bool            flip_faces;


bool
selected( uint cell )
{
    if( cell == ~0u ) {
        return false;
    }
    cell = cell & 0x0fffffffu;
    uint s = ((texelFetch( cell_subset, int(cell/32u) ).r)>>(cell%32u))&0x1u;
    return s == 1u;
}

void
main()
{
    uint cell_a = in_gs[0].cell_ix.x;
    uint cell_b = in_gs[0].cell_ix.y;
    uint o = in_gs[0].offset_a;
    uint N = in_gs[0].offset_b - o;

    bvec2 inside = bvec2( selected( cell_a), selected(cell_b ) );

    uint cell;
    bool flip;
    bool in_subset = false;
    if( inside.x && !inside.y ) {
        cell = cell_a;
        flip = false;
        in_subset = true;
    }
    else if( inside.y && !inside.x ) {
        cell = cell_b;
        flip = true;
        in_subset = true;
    }

    bool on_boundary = false;
    if( !in_subset ) {
        if( cell_a == ~0u ) {
            cell = cell_b;
            flip = true;
            on_boundary = true;
        }
        if( cell_b == ~0u ) {
            cell = cell_a;
            flip = false;
            on_boundary = true;
        }
    }

    bool on_fault = ( (cell_a != ~0u ) && (cell_b != ~0u ) && ((cell_a & 0x80000000u) != 0u ) );

    if( in_subset ) {
        cell = (cell&0x0fffffffu);
        if( flip ^^ flip_faces ) {
            cell = cell | 0x80000000u;
        }
        meta_subset = uvec4( cell, flip ? 1 : 0, o, N );
        EmitStreamVertex( 0 );
    }
    if( on_boundary ) {
        cell = (cell&0x0fffffffu);
        if( flip ^^ flip_faces ) {
            cell = cell | 0x80000000u;
        }
        meta_boundary = uvec4( cell, flip ? 1 : 0, o, N );
        EmitStreamVertex( 1 );
    }
    if( on_fault ) {
        meta_fault = uvec4( ~0u, 0, o, N );
        EmitStreamVertex( 2 );
    }
}

