/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

layout(binding=0)   uniform usamplerBuffer  cell_subset;

bool
selected( uint cell )
{
    cell = cell & 0x0fffffffu;
    uint s = ((texelFetch( cell_subset, int(cell/32u) ).r)>>(cell%32u))&0x1u;
    return s == 1u;
}

bool
predicate( out uint cell, out bool flip, in uint cell_a, in uint cell_b )
{
    if( cell_a == ~0u ) {
        if( !selected( cell_b ) ) {
            cell = cell_b;
            flip = true;
            return true;
        }
    }
    else if( cell_b == ~0u ) {
        if( !selected( cell_a ) ) {
            cell = cell_a;
            flip = false;
            return true;
        }
    }
    return false;
}
