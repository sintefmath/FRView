#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0) in uvec2 cell_ix;
layout(location=1) in uvec3 vertex_ix;
layout(location=2) in uvec3 normal_ix;

out VG {
    uvec2 cell_ix;
    uvec3 vertex_ix;
    uvec3 normal_ix;
} to_gs;

void
main()
{
    to_gs.cell_ix = cell_ix;
    to_gs.vertex_ix = vertex_ix;
    to_gs.normal_ix = normal_ix;
}
