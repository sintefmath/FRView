#version 430
/* Copyright STIFTELSEN SINTEF 2013
 *
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see http://www.gnu.org/licenses/.
 */



layout(points, invocations=1) in;
layout(points, max_vertices=1) out;

in VG {
    uvec2 cell_ix;
    uint  offset_a;
    uint  offset_b;
} in_gs[];

layout(stream=0)    out uvec4               meta_subset;
layout(stream=1)    out uvec4               meta_boundary;
layout(stream=2)    out uvec4               meta_fault;
layout(binding=0)   uniform usamplerBuffer  cell_subset;
                    uniform bool            flip_faces;

//layout(binding=0, offset=0) uniform atomic_uint counter[6];


bool
selected( uint cell )
{
    if( cell == 0x3fffffffu ) {
        return false;
    }
    uint s = ((texelFetch( cell_subset, int(cell/32u) ).r)>>(cell%32u))&0x1u;
    return s == 1u;
}

void
check( out uint cell,
       out bool flip,
       out bool in_subset,
       out bool on_boundary,
       out bool on_fault,
       in uint interface_a,
       in uint interface_b )
{
    uint cell_a = interface_a & 0x3fffffffu;
    uint cell_b = interface_b & 0x3fffffffu;
    uint convex_bit = bitfieldExtract( interface_a, 30, 1);
    on_fault = (bitfieldExtract( interface_a, 31, 1) != 0);

    bvec2 inside = bvec2( selected( cell_a), selected(cell_b ) );
    in_subset = false;
    on_boundary = false;
    if( inside.x && !inside.y ) {
        flip = false;
        in_subset = true;
    }
    else if( inside.y && !inside.x ) {
        flip = true;
        in_subset = true;
    }
    else {
        if( cell_a == 0x3fffffffu ) {
            flip = true;
            on_boundary = true;
        }
        else if( cell_b == 0x3fffffffu ) {
            flip = false;
            on_boundary = true;
        }
    }

    if( flip ) {
        cell = cell_b;
    }
    else {
        cell = cell_a;
    }
    cell = bitfieldInsert( cell, convex_bit, 30, 1 );
    if( flip ^^ flip_faces ) {
//        bitfieldInsert( cell, 1, 31, 1 );
        cell = cell | 0x80000000u;
    }
}


void
main()
{
    uint cell_a = in_gs[0].cell_ix.x;
    uint cell_b = in_gs[0].cell_ix.y;
    uint o = in_gs[0].offset_a;
    uint N = in_gs[0].offset_b - o;

    uint cell;
    bool flip, in_subset, on_boundary, on_fault;
    check( cell, flip, in_subset, on_boundary, on_fault, cell_a, cell_b );

   
    if( in_subset ) {
        //uint offset = atomicCounterIncrement( counter[0] );

        meta_subset = uvec4( cell, flip ? 1 : 0, o, N );
        EmitStreamVertex( 0 );
    }
    else if( on_boundary ) {
        //uint offset = atomicCounterIncrement( counter[1] );
        meta_boundary = uvec4( cell, flip ? 1 : 0, o, N );
        EmitStreamVertex( 1 );
    }
    if( on_fault ) {
        //uint offset = atomicCounterIncrement( counter[2] );
        meta_fault = uvec4( 0x20000000u, 0, o, N );
        EmitStreamVertex( 2 );
    }
}

