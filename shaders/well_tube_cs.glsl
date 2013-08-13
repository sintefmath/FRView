#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(vertices=2) out;

in VC {
    vec3 position;
    vec3 tangent;
    vec3 normal;
    vec3 color;
} prev[];

out CE {
    vec3 position;
    vec3 tangent;
    vec3 normal;
    vec3 binormal;
} next[];

patch out vec3 color;

void
main()
{
    next[ gl_InvocationID ].position = prev[ gl_InvocationID ].position;
    next[ gl_InvocationID ].tangent  = prev[ gl_InvocationID ].tangent;
    next[ gl_InvocationID ].normal   = normalize( prev[ gl_InvocationID ].normal );
    next[ gl_InvocationID ].binormal = normalize( cross( prev[ gl_InvocationID ].tangent,
                                                         prev[ gl_InvocationID ].normal ) );

    if( gl_InvocationID == 0 ) {
        color = prev[0].color;


        gl_TessLevelInner[ 0 ] = 40;
        gl_TessLevelInner[ 1 ] = 15;
        gl_TessLevelOuter[ 0 ] = 15;
        gl_TessLevelOuter[ 1 ] = 40;
        gl_TessLevelOuter[ 2 ] = 15;
        gl_TessLevelOuter[ 3 ] = 40;
    }

}
