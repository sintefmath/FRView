/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0)  out vec4                frag_color;
layout(location=1)  out float               complexity;
                    uniform bool            solid_pass;

void main(void)
{
    vec4 color = colorize();
    if( !(solid_pass ^^ (color.a < 0.99999) ) ) {
        discard;
    }
    else if(color.a < 0.00001 ) {
        discard;
    }

    frag_color = color;
    complexity = 1;
}
