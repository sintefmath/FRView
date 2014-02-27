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

layout(binding=0, rgba32f)  uniform imageBuffer  fragment_rgba;
layout(binding=1, rg32i)    uniform iimageBuffer fragment_node;
layout(binding=2, r32i)     uniform iimage2D     fragment_head;



void main(void)
{
    int head = imageLoad( fragment_head, ivec2( gl_FragCoord.xy) ).x;
    if( head != -1 ) {
        float processed_depth = -1e6;

        frag_color = vec4( 0.f, 0.f, 0.f, 1.f );
        for(int q=0; q<100; q++ ) {
            
            float current_depth = 1e6;
            int   current_index = head;
            
            int ix = head;
            do {
                ivec2 node = imageLoad( fragment_node, ix ).xy;
                float depth = intBitsToFloat( node.y );
                if( (depth > processed_depth) && (depth < current_depth ) ) {
                    current_depth = depth;
                    current_index = ix;
                } 
                ix = node.x;
            } while( ix != -1 );
            
            if( current_depth > 1.0 ) {
                break;
            }
            vec4 fragment = imageLoad( fragment_rgba, current_index );
           
            // fragments are premultiplied
            frag_color.rgb = frag_color.rgb + frag_color.a*fragment.rgb;
            frag_color.a   = frag_color.a*(1.f-fragment.a);
            if( frag_color.a < 0.01f ) {
                break;
            }
            processed_depth = current_depth;
        }
    }
    else {
        discard;
    }
}
