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
#ifdef DO_PAINT
    flat vec3    boundary_a;
    flat vec3    boundary_b;
    flat vec3    boundary_c;
#endif
    smooth vec3  normal;
    smooth vec3  obj_pos;
} in_f;

                    uniform vec4            edge_color;
                    uniform bool            solid_pass;

layout(binding=0, r32i)     uniform image2D         heads;
layout(binding=0, offset=0) uniform atomic_int     alloc;

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

#ifdef DO_PAINT
    if( edge_color.w > 0.f ) {
        float line = min(abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_a )),
                         min(abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_b )),
                             abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_c )) ) );

        if( line < 1.0 ) {
            float alpha = edge_color.w *max(0.0, 1.0-line);
            color += vec4( alpha*edge_color.rgb, alpha );
            //color += vec4(edge_color*max(0.f, 1.0-line ));
        }
    }
#endif
    if( !(solid_pass ^^ (color.a < 0.99999) ) ) {
        discard;
    }
    else if(color.a < 0.00001 ) {
        discard;
    }

}
