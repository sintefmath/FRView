#version 330
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

uniform uvec3           grid_dim;
uniform uvec3           index_min;
uniform uvec3           index_max;
uniform usamplerBuffer  cell_global_index;

out uint                selected;

bool
inSubset( int cell )
{
    uint global = texelFetch( cell_global_index, cell ).r;

    
    if( grid_dim.z != 0u ) {
        uvec3 index = uvec3( global % grid_dim.x,
                             (global/grid_dim.x) % grid_dim.y,
                             (global/grid_dim.x)/grid_dim.y );
        return all(lessThanEqual( index_min, index )) && all(lessThanEqual( index, index_max ));
    }
    else {
        return (index_min.x <= global) && (global <= index_max.x );
    }
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
//    selected = inSubset( gl_VertexID ) ? 1u : 0u;
}
