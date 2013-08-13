#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0) in uvec2 cornerpoint_ix;
layout(location=1) in uvec4 cell_ix;

out VG {
    uvec2 cornerpoint_ix;
    uvec4 cell_ix;
} to_gs;

void
main()
{
    to_gs.cornerpoint_ix = cornerpoint_ix;
    to_gs.cell_ix = cell_ix;
}
