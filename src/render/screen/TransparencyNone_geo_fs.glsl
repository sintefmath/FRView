/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/



layout(location=0)  out vec3                frag_color;

void main(void)
{
    vec4 color = colorize();
    if( color.a < 0.9f ) {
        discard;
    }
    frag_color = color.rgb;
}
