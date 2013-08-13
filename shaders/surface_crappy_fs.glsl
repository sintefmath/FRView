#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0)  out vec4                frag_color;
                    uniform vec3            solid_color;

void main(void)
{
    frag_color.rgb = solid_color;
}
