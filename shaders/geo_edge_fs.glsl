#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0)  out     vec4    frag_color;
                    uniform vec4    edge_color;

void main(void)
{
    frag_color = edge_color;
}
