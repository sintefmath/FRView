#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
layout(triangles, invocations=1) in;
layout(triangle_strip, max_vertices=3) out;

in GI {
    vec3    obj_pos;
    vec2    scr_pos;
} in_g[3];

out GO {
    flat vec4    color;
    flat vec3    boundary_a;
    flat vec3    boundary_b;
    flat vec3    boundary_c;
    smooth vec3  normal;
    smooth vec3  obj_pos;
} out_g;

layout(binding=0)   uniform usamplerBuffer  cells;
layout(binding=1)   uniform samplerBuffer   normals;
layout(binding=2)   uniform samplerBuffer   field;
                    uniform mat3            NM;
                    uniform float           solid_alpha;
                    uniform float           line_alpha;
                    uniform vec2            field_remap;
                    uniform bool            use_field;
                    uniform bool            flat_normals;
                    uniform bool            log_map;
                    uniform bool            solid_pass;
                    uniform vec3            solid_color;

void
main()
{
    vec2 da = normalize( in_g[1].scr_pos - in_g[0].scr_pos );
    vec2 db = normalize( in_g[2].scr_pos - in_g[1].scr_pos );
    vec2 dc = normalize( in_g[0].scr_pos - in_g[2].scr_pos );


    uint cell;
    vec3 n0, n1, n2;
    if( false && flat_normals ) {
        cell = texelFetch( cells, gl_PrimitiveIDIn ).r;
        n0 = cross( in_g[1].obj_pos - in_g[0].obj_pos,
                       in_g[2].obj_pos - in_g[0].obj_pos);
        n1 = n0;
        n2 = n0;
    }
    else {
        uvec4 indices = texelFetch( cells, gl_PrimitiveIDIn );
        cell = indices.r;
        float s = ((indices.y & 0x40000000u) != 0u ) ? 1.f : -1.f;
        n0 = normalize( s*NM*texelFetch( normals, int(indices.y & 0x0fffffffu) ).rgb );
        n1 = normalize( s*NM*texelFetch( normals, int(indices.z & 0x0fffffffu) ).rgb );
        n2 = normalize( s*NM*texelFetch( normals, int(indices.w & 0x0fffffffu) ).rgb );
    }

    vec3 color;
    if( use_field ) {
        uint cid = cell & 0x0fffffffu;
        float value = texelFetch( field, int(cid) ).r;
        if( log_map ) {
            value = log( value );
        }
        float scalar = clamp( field_remap.y*( value - field_remap.x), 0.0, 1.0 );
        color = max( vec3(0.f),
                         sin( vec3( 4.14f*(scalar - 0.5f),
                                    3.14f*(scalar),
                                    4.14f*(scalar + 0.5f) ) ) );
    }
    else {
//        color.rgb = solid_color;
        uint cid = cell & 0x0fffffffu;
//        uint cid = uint(gl_PrimitiveIDIn);//cell & 0x0fffffffu;
        color.rgb =
                vec3(  0.2*float( int( cid & 8u  ) == 0 ) + 0.2*float( int( cid & 1u ) == 0  ) + 0.2,
                       0.2*float( int( cid & 16u ) == 0 ) + 0.2*float( int( cid & 2u ) == 0  ) + 0.2,
                       0.2*float( int( cid & 32u ) == 0 ) + 0.2*float( int( cid & 4u ) == 0  ) + 0.2 );

    }


    out_g.color = solid_alpha*vec4( color, 1.f);

    if(  (cell & 0x40000000u) != 0u ) {
        out_g.boundary_a.xy = vec2( -da.y, da.x );
        out_g.boundary_a.z = -dot( out_g.boundary_a.xy, in_g[0].scr_pos );
    }
    else {
        out_g.boundary_a.xy = vec2( 0.f );
        out_g.boundary_a.z = 1.f;
    }

    if(  (cell & 0x20000000u) != 0u ) {
        out_g.boundary_b.xy = vec2( -db.y, db.x );
        out_g.boundary_b.z = -dot( out_g.boundary_b.xy, in_g[1].scr_pos );
    }
    else {
        out_g.boundary_b.xy = vec2( 0.f );
        out_g.boundary_b.z = 1.f;
    }

    if( (cell & 0x10000000u) != 0u ) {
        out_g.boundary_c.xy = vec2( -dc.y, dc.x );
        out_g.boundary_c.z = -dot( out_g.boundary_c.xy, in_g[2].scr_pos );
    }
    else {
        out_g.boundary_c.xy = vec2( 0.f );
        out_g.boundary_c.z = 1.f;
    }

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
