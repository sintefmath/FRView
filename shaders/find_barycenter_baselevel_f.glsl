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

                    uniform float           weight; // =1/n
layout(location=0)  out vec3                buffer_barycenter;

void main(void)
{
    buffer_barycenter = vec3(weight)*in_f.position;
}
