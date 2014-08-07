#version 420
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
layout(points, invocations=1) in;
layout(points, max_vertices=MAX_OUT) out;

// recorded into xfb 0
layout(stream=0) out vec4    out_triangle_color;
layout(stream=0) out vec3    out_triangle_normal;
layout(stream=0) out vec3    out_triangle_position;

// recorded into xfb 1
layout(stream=1) out vec4    out_edge_position;

#define POSITIONS_DECLARED
layout(binding=3)   uniform samplerBuffer   positions;
layout(binding=4)   uniform usamplerBuffer  cells;
layout(binding=5)   uniform samplerBuffer   normals;
layout(binding=6)   uniform samplerBuffer   field;
layout(binding=7)   uniform sampler1D       color_map;
                    uniform mat3            NM;
                    uniform vec2            field_remap;
                    uniform bool            use_field;
                    uniform bool            flat_normals;
                    uniform bool            log_map;
                    uniform bool            solid_pass;
                    uniform vec4            surface_color;


void
emit_triangle( in uint cell_ix,
               in uvec3 nrm_ix,
               in uvec3 vtx_ix )
{
    vec3 pa = texelFetch( positions, int(vtx_ix.x) ).rgb;
    vec3 pb = texelFetch( positions, int(vtx_ix.y) ).rgb;
    vec3 pc = texelFetch( positions, int(vtx_ix.z) ).rgb;

    out_triangle_color = vec4( 1.f, 0.f, 0.f, 0.5f );
    out_triangle_normal = texelFetch( normals, int(nrm_ix.x & 0x0fffffffu) ).rgb;
    out_triangle_position = pa;
    EmitStreamVertex( 0 );


    out_triangle_color = vec4( 0.f, 1.f, 0.f, 0.5f );
    out_triangle_normal = texelFetch( normals, int(nrm_ix.y & 0x0fffffffu) ).rgb;
    out_triangle_position = pb;
    EmitStreamVertex(0);


    out_triangle_color = vec4( 0.f, 0.f, 1.f, 0.5f );
    out_triangle_normal = texelFetch( normals, int(nrm_ix.z & 0x0fffffffu) ).rgb;
    out_triangle_position = pc;
    EmitStreamVertex(0);

    if( ((nrm_ix.x & 0x80000000u) != 0u) && (vtx_ix.x < vtx_ix.y ) ) {
        out_edge_position = vec4(pa, 1.f);
        EmitStreamVertex(1);

        out_edge_position = vec4(pb, 1.f);
        EmitStreamVertex(1);
    }

    if( ((nrm_ix.y & 0x80000000u) != 0u) && (vtx_ix.y < vtx_ix.z ) ) {
        out_edge_position = vec4(pb, 1.f);
        EmitStreamVertex(1);

        out_edge_position = vec4(pc, 1.f);
        EmitStreamVertex(1);
    }

    if( ((nrm_ix.z & 0x80000000u) != 0u) && (vtx_ix.z < vtx_ix.x ) ) {
        out_edge_position = vec4(pc, 1.f);
        EmitStreamVertex(1);

        out_edge_position = vec4(pa, 1.f);
        EmitStreamVertex(1);
    }


}
