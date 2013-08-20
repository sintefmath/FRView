#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

uniform uvec3           grid_dim;
uniform uvec3           index_min;
uniform uvec3           index_max;
uniform usamplerBuffer  cell_global_index;

out uint                selected;

bool
inSubset( int cell )
{
    uint global = texelFetch( cell_global_index, cell ).r;

    uvec3 index = uvec3( global % grid_dim.x,
                         (global/grid_dim.x) % grid_dim.y,
                         (global/grid_dim.x)/grid_dim.y );

    return all(lessThanEqual( index_min, index )) && all(lessThanEqual( index, index_max ));
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
