#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

layout(location=0)  in      vec3 bbmin;
layout(location=1)  in      vec3 bbmax;
layout(location=2)  in      uint cell;

out GI {
    vec3 bbmin;
    vec3 bbmax;
    uint cell;
} gi;

void main()
{
    gi.bbmin = vec3(2.f,2.f,1.f)*bbmin - vec3(1.f,1.f,0.f);
    gi.bbmax = vec3(2.f,2.f,1.f)*bbmax - vec3(1.f,1.f,0.f);
//    gi.bbmin = vec3(2.f,1.f,1.f)*bbmin - vec3(1.f,1.f,0.f);
//    gi.bbmax = vec3(2.f,1.f,1.f)*bbmax - vec3(1.f,1.f,0.f);
    gi.cell  = cell+1; // 0 is reserved for empty space

}
