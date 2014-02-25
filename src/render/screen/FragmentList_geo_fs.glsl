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

layout(binding=0, offset=0) uniform atomic_uint  fragment_counter;
                            uniform int          fragment_alloc;
layout(binding=0, rgba32f)  uniform imageBuffer  fragment_rgba;
layout(binding=1, rg32i)    uniform iimageBuffer fragment_node;
layout(binding=2, r32i)     uniform iimage2D     fragment_head;

void main(void)
{
    vec4 color = colorize();
    if( color.a > 0.0f ) {
        int ix = int( atomicCounterIncrement( fragment_counter ) );
        if( ix < fragment_alloc ) {
            int next = imageAtomicExchange( fragment_head, ivec2(gl_FragCoord.xy), ix );
            imageStore( fragment_rgba, ix, color );
            imageStore( fragment_node, ix, ivec4(next, floatBitsToInt( gl_FragCoord.z ), 0.f, 0.f ) );
        }
    }
}
