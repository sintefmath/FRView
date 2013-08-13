/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/


bool
predicate( out uint cell, out bool flip, in uint cell_a, in uint cell_b )
{
    if( (cell_a != ~0u ) && (cell_b != ~0u ) && ((cell_a & 0x80000000u) != 0u ) ) {
        flip = false;
        cell = cell_a | cell_b;
        return true;
    }
    else {
        return false;
    }
}
