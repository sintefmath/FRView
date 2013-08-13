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
    flat vec4    color;
    flat vec3    boundary_a;
    flat vec3    boundary_b;
    flat vec3    boundary_c;
    smooth vec3  normal;
    smooth vec3  obj_pos;
} in_f;

layout(location=0)  out vec4                frag_color;
layout(location=1)  out float               complexity;
                    uniform float           line_alpha;
                    uniform bool            solid_pass;
                    uniform bool            low_quality;


void main(void)
{
    vec3 v = normalize( vec3( 0.f, 0.f, 1.5f ) - in_f.obj_pos ); // towards eye
    vec3 l = normalize( vec3( 1.5f, 1.5f, 1.5f ) - in_f.obj_pos );  // towards light
    vec3 h = normalize( v + l );                  // half-vector
    vec3 n = normalize( in_f.normal );
    float d = max( 0.5f, dot(n,l) );
    float s = pow( max( 0.f, dot(n,h) ), 50.f );

    vec4 color = in_f.color;
    color.rgb = d*color.rgb + vec3(s);
//    color.rgb = vec3( 0.1 + s);

//    color.rgb = vec3(0.5) + 0.5*n;

//   out_g.color = solid_alpha*vec4( max(0.2, normal.z)*color, 1.f);


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
