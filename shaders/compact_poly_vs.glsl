#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#if 0
layout(location=0) in uvec2 cell_ix;
layout(location=1) in uint  offset_a;
layout(location=2) in uint  offset_b;

out VG {
    uvec2 cell_ix;
    uint  offset_a;
    uint  offset_b;
} to_gs;


void
main()
{
    to_gs.cell_ix = cell_ix;
    to_gs.offset_a = offset_a;
    to_gs.offset_b = offset_b;
}
#else
layout(location=0) in uvec4 meta;
out VG {
    uvec4 meta;
} to_gs;
void
main()
{
    to_gs.meta = meta;
}
#endif
