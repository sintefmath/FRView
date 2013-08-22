#version 330
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0) in vec4 position;

out vec2 normalized;

void
main()
{
    normalized = 0.5f*(position.xy + vec2(1.f) );
    gl_Position = position;
}
