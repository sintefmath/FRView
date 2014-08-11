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


layout(triangles, invocations=1) in;
layout(triangle_strip, max_vertices=3) out;

in GI {
    vec3    obj_pos;
    vec2    scr_pos;
} in_g[3];

out GO {
    flat bool    two_sided;
    flat vec4    color;
#ifdef DO_PAINT
    flat vec3    boundary_a;
    flat vec3    boundary_b;
    flat vec3    boundary_c;
#endif
    smooth vec3  normal;
    smooth vec3  obj_pos;
} out_g;

layout(binding=0)   uniform usamplerBuffer  cells;
layout(binding=1)   uniform samplerBuffer   normals;
layout(binding=2)   uniform samplerBuffer   field;
layout(binding=3)   uniform sampler1D       color_map;
                    uniform mat3            NM;
                    uniform vec2            field_remap;
                    uniform bool            use_field;
                    uniform bool            flat_normals;
                    uniform bool            log_map;
                    uniform bool            solid_pass;
                    uniform vec4            surface_color;

void
main()
{
    // fetch meta data
    uvec4 indices = texelFetch( cells, gl_PrimitiveIDIn );
    uint cell = indices.r;

    // fetch and transform normal vectors
    vec3 n0 = normalize( NM*texelFetch( normals, int(indices.y & 0x0fffffffu) ).rgb );
    vec3 n1 = normalize( NM*texelFetch( normals, int(indices.z & 0x0fffffffu) ).rgb );
    vec3 n2 = normalize( NM*texelFetch( normals, int(indices.w & 0x0fffffffu) ).rgb );


    bool flip = false;
    if( (cell & 0x80000000u) == 0u ) {
        flip = !flip;
    }

    if( flip ) {
        n0 = -n0;
        n1 = -n1;
        n2 = -n2;
    }
    
    vec3 color;
    if( use_field ) {
        // colorize using a field
        uint cid = cell & 0x0fffffffu;
        float value = texelFetch( field, int(cid) ).r;
        if( log_map ) {
            // field_remap.x = 1.0/min_value
            // field_remap.y = 1.0/log(max_value/min_value)
            value = log( value*field_remap.x )*field_remap.y;
        }
        else {
            // field_remap.x = min_value
            // field_remap.y = 1.0/(max_value-min_value)
            value = field_remap.y*( value - field_remap.x);
        }
        color = texture( color_map, value ).rgb;
    }
    else {
        color = surface_color.rgb;
    }

    out_g.two_sided = bitfieldExtract( cell, 29, 1 ) == 1u;
    out_g.color = surface_color.w*vec4( color, 1.0 );
#ifdef DO_PAINT
    vec2 da = normalize( in_g[1].scr_pos - in_g[0].scr_pos );
    vec2 db = normalize( in_g[2].scr_pos - in_g[1].scr_pos );
    vec2 dc = normalize( in_g[0].scr_pos - in_g[2].scr_pos );
    if( (indices.y & 0x80000000u) != 0u ) {
//    if(  (cell & 0x40000000u) != 0u ) {
        out_g.boundary_a.xy = vec2( -da.y, da.x );
        out_g.boundary_a.z = -dot( out_g.boundary_a.xy, in_g[0].scr_pos );
    }
    else {
        out_g.boundary_a.xy = vec2( 0.f );
        out_g.boundary_a.z = 100000.f;
    }

    if( (indices.z & 0x80000000u) != 0u ) {
//    if(  (cell & 0x20000000u) != 0u ) {
        out_g.boundary_b.xy = vec2( -db.y, db.x );
        out_g.boundary_b.z = -dot( out_g.boundary_b.xy, in_g[1].scr_pos );
    }
    else {
        out_g.boundary_b.xy = vec2( 0.f );
        out_g.boundary_b.z = 100000.f;
    }

    if( (indices.w & 0x80000000u) != 0u ) {
//    if( (cell & 0x10000000u) != 0u ) {
        out_g.boundary_c.xy = vec2( -dc.y, dc.x );
        out_g.boundary_c.z = -dot( out_g.boundary_c.xy, in_g[2].scr_pos );
    }
    else {
        out_g.boundary_c.xy = vec2( 0.f );
        out_g.boundary_c.z = 100000.f;
    }
#endif
    out_g.obj_pos = in_g[0].obj_pos;
    out_g.normal = n0;
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    out_g.obj_pos = in_g[1].obj_pos;
    out_g.normal = n1;
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    out_g.obj_pos = in_g[2].obj_pos;
    out_g.normal = n2;
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
}
