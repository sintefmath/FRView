#version 330
#extension GL_ARB_shading_language_420pack : enable
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(location=0)  out vec4                frag_color;
layout(binding=0)   uniform sampler2D       font;

in FS {
    vec2                    tpos;
}                           in_fs;

void main(void)
{
    float i = texelFetch( font, ivec2( in_fs.tpos ), 0 ).r;
    if( i == 0.f ) {
        discard;
    }
    frag_color = vec4( i );
}
