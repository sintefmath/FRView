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
    if( cell_a == ~0u ) {
        cell = cell_b;
        flip = true;
        return true;
    }
    else if( cell_b == ~0u ) {
        cell = cell_a;
        flip = false;
        return true;
    }
    else {
        return false;
    }
}
