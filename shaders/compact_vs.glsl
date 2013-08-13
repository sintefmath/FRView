#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0) in uvec2 info;
layout(location=1) in uvec3 indices;

out VG {
    uvec2 info;
    uvec3 indices;
} to_gs;

void
main()
{
    to_gs.info = info;
    to_gs.indices = indices;
}
