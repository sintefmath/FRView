#version 420
/* Copyright STIFTELSEN SINTEF 2013
 *
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see http://www.gnu.org/licenses/.
 */



layout(location=0)  out vec4            frag_color;

//layout(binding=0)   uniform sampler2D   tex_solid_color;
//layout(binding=1)   uniform sampler2D   tex_transparent_color;
//layout(binding=2)   uniform sampler2D   tex_transparent_complexity;

layout(binding=0, rgba32f)  uniform imageBuffer  fragment_rgba;
layout(binding=1, rg32i)    uniform iimageBuffer fragment_node;
layout(binding=2, r32i)     uniform iimage2D     fragment_head;

void main(void)
{
    int head = imageLoad( fragment_head, ivec2( gl_FragCoord.xy) ).x;
    if( head != -1 ) {
        float near = 1.0;
        
        int ix = head;
        int best_ix = ix;
        do {
            ivec2 node = imageLoad( fragment_node, ix ).xy;
            float depth = intBitsToFloat( node.y );
            if( depth < near ) {
                near = depth;
                best_ix = ix;
            }
            ix = node.x;
        } while( ix != -1 );
        
        frag_color = imageLoad( fragment_rgba, best_ix );
    }
    else {
        discard;
    }
}
