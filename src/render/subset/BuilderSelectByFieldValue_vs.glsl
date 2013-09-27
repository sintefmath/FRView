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
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */


                    out uint                selected;
layout(binding=0)   uniform samplerBuffer   field;
                    uniform vec2            min_max;

bool
inSubset( int cell )
{
    float value = texelFetch( field, int(cell) ).r;
    return min_max.x <= value && value <= min_max.y;
}

void
main()
{
    uint s = 0u;
    for(int i=0; i<32; i++) {
        if( inSubset( 32*gl_VertexID + i ) ) {
            s |= (1u<<uint(i));
        }
    }
    selected = s;
}
