#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0) in vec4 position;

uniform mat4    MVP;

void main()
{
    gl_Position = MVP*vec4(position.xyz,1.f);
}
