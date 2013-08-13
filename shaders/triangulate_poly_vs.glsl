#version 430
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0) in uvec4 meta;

out VG {
    uvec4 meta;
} to_gs;


void
main()
{
    to_gs.meta = meta;
}
