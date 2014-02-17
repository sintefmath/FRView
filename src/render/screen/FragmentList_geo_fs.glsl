/* Copyright STIFTELSEN SINTEF 2014
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


layout(location=0)  out vec3                frag_color;

layout(binding=0, offset=0) uniform atomic_uint  fragment_counter;
                            uniform int          fragment_alloc;
layout(binding=0, rgba32f)  uniform imageBuffer  fragment_rgba;
layout(binding=1, r32f)     uniform imageBuffer  fragment_depth;
layout(binding=2, r32i)     uniform iimageBuffer fragment_next;
layout(binding=3, r32i)     uniform iimage2D     fragment_head;

void
store( vec4 rgba )
{
    int ix = int( atomicCounterIncrement( fragment_counter ) );
    if( ix < fragment_alloc ) {
        int next = imageAtomicExchange( fragment_head, ivec2(gl_FragCoord.xy), ix );
        imageStore( fragment_rgba, ix, rgba );
        imageStore( fragment_depth, ix, vec4( gl_FragDepth ) );
        imageStore( fragment_next, ix, ivec4(next) );
    }
}

void main(void)
{
    vec4 color = colorize();
    if( color.a > 0.0f ) {
        store( color );
        frag_color = color.rgb;
    }
    else {
        discard;
    }
}
