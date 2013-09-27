#version 330
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
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */



uniform samplerBuffer   vertices;
uniform usamplerBuffer  cell_corner;
uniform vec4 plane_equation;

out uint selected;

bool
inSubset( int cell )
{
    uvec4 corner0 = texelFetch( cell_corner, 2*int(cell)+0 );
    uvec4 corner1 = texelFetch( cell_corner, 2*int(cell)+1 );
    bvec4 inside0 = bvec4( dot( texelFetch( vertices, int(corner0.x) ), plane_equation ) >= 0.f,
                           dot( texelFetch( vertices, int(corner0.y) ), plane_equation ) >= 0.f,
                           dot( texelFetch( vertices, int(corner0.z) ), plane_equation ) >= 0.f,
                           dot( texelFetch( vertices, int(corner0.w) ), plane_equation ) >= 0.f );

    bvec4 inside1 = bvec4( dot( texelFetch( vertices, int(corner1.x) ), plane_equation ) >= 0.f,
                           dot( texelFetch( vertices, int(corner1.y) ), plane_equation ) >= 0.f,
                           dot( texelFetch( vertices, int(corner1.z) ), plane_equation ) >= 0.f,
                           dot( texelFetch( vertices, int(corner1.w) ), plane_equation ) >= 0.f );

    if( all(inside0) && all(inside1) ) {
        return false;
    }
    else if( !any(inside0) && !any(inside1) ) {
        return false;
    }
    else {
        return true;
    }
}


void
main()
{
    uint s = 0u;
    for(int i=0; i<32; i++) {
        if( inSubset( 32*gl_VertexID + i ) ) {
            s |= (1u<<uint(i));
        }
    }
    selected = s;
//    selected = inSubset( gl_VertexID ) ? 1u : 0u;
}
