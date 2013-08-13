#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0) in vec3 position;
layout(location=1) in vec3 tangent;
layout(location=2) in vec3 normal;
layout(location=3) in vec3 color;


uniform mat4 world_from_model;
uniform mat3 world_from_model_nm;

out VC {
    vec3 position;
    vec3 tangent;
    vec3 normal;
    vec3 color;
} next;

void
main()
{
    next.color = color;

    // The usual transform for directions is to use the inverse transpose
    // matrix. However, this preserves directions but the vector gets an insane
    // scaling. Which is not what we want for tangents.
    vec4 ph = world_from_model * vec4( position, 1.f );
    vec4 th = world_from_model * vec4( position + tangent, 1.f );
    vec4 nh = world_from_model * vec4( position + normal, 1.f );

    next.position = (1.f/ph.w)*ph.xyz;
    next.tangent  = (1.f/th.w)*th.xyz - (1.f/ph.w)*ph.xyz;
    next.normal   = (1.f/nh.w)*nh.xyz - (1.f/ph.w)*ph.xyz;

    // Non-uniform scale destroys the orthogonalness of tangent and normal, so
    // we make it orthogonal.
    next.normal   = normalize( next.normal - (dot(next.normal, next.tangent)/dot(next.tangent,next.tangent))*next.tangent );
}
