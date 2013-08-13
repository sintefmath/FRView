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

layout(binding=0)   uniform usamplerBuffer  cell_subset;

in VG {
    uvec2 cornerpoint_ix;
    uvec4 cell_ix;
} in_gs[];

layout(location=0)  out uvec2  cornerpoint_ix;

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



#ifdef COMPACT_SUBSET

#if 1
// predicate that extracts the geometric outline of the subset (not cell outlines)
bool
predicate( uvec2 ends, uvec4 cells ) {
    bvec2 eqmask = bvec2( cells.x != cells.y, cells.z != cells.w );
    bvec4 mask = bvec4( selected(cells.x), selected(cells.y), selected(cells.z), selected(cells.w ) );
    float cnt = dot( vec4(1), vec4(mask));
    return ((cnt == 1) || (cnt==3));// && ((gl_PrimitiveIDIn % 8) == 1);
}
#else
bool
predicate( uvec2 ends, uvec4 cells ) {
    bvec2 eqmask = bvec2( cells.x != cells.y, cells.z != cells.w );
    bvec4 mask = bvec4( selected(cells.x), selected(cells.y), selected(cells.z), selected(cells.w ) );
    bvec4 mask2 = eqmask.xxyy && mask;
    return any( mask2 ) && !all( mask );
}
#endif

#endif
#ifdef COMPACT_FAULT
bool
predicate( uvec2 ends, uvec4 cells ) {
    return all( equal( cells, uvec4(~0u) ) );
}
/*bool
predicate( uvec2 ends, uvec4 cells ) {
    uint tmp = cells.x & cells.y & cells.z & cells.w;
    return all(notEqual( cells, uvec4(~0u) )) &&  (tmp>>31u) != 0u;
}
*/
#endif
#ifdef COMPACT_BOUNDARY
bool
predicate( uvec2 ends, uvec4 cells ) {
    return all( equal( cells, uvec4(~0u) ) );
}
#endif



void
main()
{
    if( predicate( in_gs[0].cornerpoint_ix, in_gs[0].cell_ix ) ) {
        cornerpoint_ix = in_gs[0].cornerpoint_ix;
        EmitVertex();
    }
}

