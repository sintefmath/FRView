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

layout(location=0)  out vec3                frag_color;
                    uniform float           line_alpha;

void main(void)
{
    vec4 color = in_f.color;
    if( line_alpha > 0.f ) {
        float line = min(abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_a )),
                         min(abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_b )),
                             abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_c )) ) );

        if( line < 1.0 ) {
            color = vec4( 1.0 );
        }
    }
    if( color.a < 0.9f ) {
        discard;
    }
    frag_color = color.rgb;
}
