#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

in GO {
    flat vec4   color;
    flat vec3   boundary_a;
    flat vec3   boundary_b;
    flat vec3   boundary_c;
} in_f;

layout(location=0)  out vec4                frag_color;
layout(location=1)  out float               complexity;
                    uniform float           line_alpha;
                    uniform bool            solid_pass;
                    uniform bool            low_quality;


void main(void)
{
    vec4 color = in_f.color;
    if( line_alpha > 0.f ) {
        float line = min(abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_a )),
                         min(abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_b )),
                             abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_c )) ) );

        if( line < 1.0 ) {
            color += vec4(line_alpha*max(0.f, 1.0-line ));
        }
    }
    if( !(solid_pass ^^ (color.a < 0.99999) ) ) {
        discard;
    }
    else if(color.a < 0.00001 ) {
        discard;
    }

    frag_color = color;
    complexity = 1;
}
