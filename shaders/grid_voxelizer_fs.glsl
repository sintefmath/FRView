#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
in FI {
    flat uint cell;
} fi;

layout(location=0)  out uint  frag_color;

void main(void)
{
    frag_color = fi.cell;
}
