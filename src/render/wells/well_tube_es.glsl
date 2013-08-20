#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout( quads ) in;

in CE {
    vec3 position;
    vec3 tangent;
    vec3 normal;
    vec3 binormal;
} prev[];

patch in vec3 color;

out EF {
    vec2 param;
    vec3 normal;
    vec3 color;
} next;

#define M_2PI 6.28318530717958647692528676655900576

uniform mat4    projection;
uniform mat4    modelview;

void
main()
{


    float u = M_2PI*gl_TessCoord.x;
    float v = gl_TessCoord.y;


    vec3 a = normalize( mix( prev[0].normal, prev[1].normal, v ) );
    vec3 b = normalize( mix( prev[0].binormal, prev[1].binormal, v ) );
    vec3 d = cos( u )*a + sin( u )*b;

//    vec3 c = mix( prev[0].position, prev[1].position, v );


    vec3 c = ( 2.f*v*v*v - 3.f*v*v + 1.f )*prev[0].position
           + (     v*v*v - 2.f*v*v + v   )*(prev[0].tangent )
           + (-2.f*v*v*v + 3.f*v*v       )*prev[1].position
           + (     v*v*v -     v*v       )*(prev[1].tangent);



    next.normal = (modelview*vec4(d,0.f)).xyz;
    next.param = gl_TessCoord.xy;
    next.color = color;

    gl_Position = projection*(modelview*vec4(c + 0.003*d,1.f));

}
