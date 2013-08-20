#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

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

    return any(inside0) || any(inside1);
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
