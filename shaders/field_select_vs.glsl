#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

                    out uint                selected;
layout(binding=0)   uniform samplerBuffer   field;
                    uniform vec2            min_max;

bool
inSubset( int cell )
{
    float value = texelFetch( field, int(cell) ).r;
    return min_max.x <= value && value <= min_max.y;
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
}
