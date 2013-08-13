#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

in FI {
    flat vec3 position;
} in_f;

                    uniform float           weight; // = sqrt(1/n)
                    uniform vec3            barycenter; // can use texlookup
layout(location=0)  out vec3                buffer_cov0;
layout(location=1)  out vec3                buffer_cov1;

void main(void)
{
    vec3 p = weight*( in_f.position - barycenter );

    buffer_cov0 = vec3( p.x*p.x,
                        p.x*p.y,
                        p.x*p.z );
    buffer_cov1 = vec3( p.y*p.y,
                        p.y*p.z,
                        p.z*p.z );
}
