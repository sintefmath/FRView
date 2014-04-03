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


layout(points, invocations=1) in;
layout(points, max_vertices=1) out;

// data to capture
out vec3 bbmin;
out vec3 bbmax;
out uint cellid;

layout(binding=0)   uniform usamplerBuffer  cell_subset;
layout(binding=1)   uniform samplerBuffer   vertices;
layout(binding=2)   uniform usamplerBuffer  cell_corner;
                    uniform mat4            local_to_world;

bool
selected( uint cell )
{
    if( cell == ~0u ) {
        return false;
    }
    cell = cell & 0x0fffffffu;
    uint s = ((texelFetch( cell_subset, int(cell/32u) ).r)>>(cell%32u))&0x1u;
    return s == 1u;
}

void main()
{
    uint cell = gl_PrimitiveIDIn;

    vec2 cmin = vec2(-2.f);
    vec2 cmax = vec2(-2.f);
    if( selected( cell ) ) {
        uvec4 corner0 = texelFetch( cell_corner, 2*int(cell)+0 );
        uvec4 corner1 = texelFetch( cell_corner, 2*int(cell)+1 );

        // assume that local_to_world is plain affine transformation
        vec3 v0 = (local_to_world*texelFetch( vertices, int(corner0.x) )).xyz;
        vec3 v1 = (local_to_world*texelFetch( vertices, int(corner0.y) )).xyz;
        vec3 v2 = (local_to_world*texelFetch( vertices, int(corner0.z) )).xyz;
        vec3 v3 = (local_to_world*texelFetch( vertices, int(corner0.w) )).xyz;
        vec3 v4 = (local_to_world*texelFetch( vertices, int(corner1.x) )).xyz;
        vec3 v5 = (local_to_world*texelFetch( vertices, int(corner1.y) )).xyz;
        vec3 v6 = (local_to_world*texelFetch( vertices, int(corner1.z) )).xyz;
        vec3 v7 = (local_to_world*texelFetch( vertices, int(corner1.w) )).xyz;


        vec3 vmin01 = min( v0, v1 );
        vec3 vmin23 = min( v2, v3 );
        vec3 vmin45 = min( v4, v5 );
        vec3 vmin67 = min( v6, v7 );
        vec3 vmin0123 = min( vmin01, vmin23 );
        vec3 vmin4567 = min( vmin45, vmin67 );
        vec3 vmin = min( vmin0123, vmin4567 );

        vec3 vmax01 = max( v0, v1 );
        vec3 vmax23 = max( v2, v3 );
        vec3 vmax45 = max( v4, v5 );
        vec3 vmax67 = max( v6, v7 );
        vec3 vmax0123 = max( vmax01, vmax23 );
        vec3 vmax4567 = max( vmax45, vmax67 );
        vec3 vmax = max( vmax0123, vmax4567 );

        // Emit cell
        bbmin = clamp( vmin, vec3(0.0), vec3(1.0) );
        bbmax = clamp( vmax, vec3(0.0), vec3(1.0) );
        cellid = cell;
        EmitVertex();
    }
}

