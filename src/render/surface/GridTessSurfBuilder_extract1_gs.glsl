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
    uint  cell_ix;
    uint  offset_a;
    uint  offset_b;
} in_gs[];

layout(stream=0)    out uvec4               meta_subset;
layout(stream=1)    out uvec4               meta_boundary;
layout(stream=2)    out uvec4               meta_fault;
layout(binding=0)   uniform usamplerBuffer  cell_subset;
                    uniform bool            flip_faces;

bool
selected( uint cell )
{
    cell = cell & 0x1fffffffu;
    if( cell == 0x1fffffffu ) {
        return false;
    }
    uint s = ((texelFetch( cell_subset, int(cell/32u) ).r)>>(cell%32u))&0x1u;
    return s == 1u;
}

void
main()
{
    uint cell = in_gs[0].cell_ix;
    uint o    = in_gs[0].offset_a;
    uint N    = in_gs[0].offset_b - o;

    cell = cell | 0x20000000u;
    
    bool flip = true;
    if( /*flip ^^*/ flip_faces ) {
//        bitfieldInsert( cell, 1, 31, 1 );
        cell = cell | 0x80000000u;
    }
    if( selected( cell ) ) {
        meta_subset = uvec4( cell, flip ? 1 : 0, o, N );
        EmitStreamVertex( 0 );
    }
    else {
        meta_boundary = uvec4( cell, flip ? 1 : 0, o, N );
        EmitStreamVertex( 1 );
    }
}

