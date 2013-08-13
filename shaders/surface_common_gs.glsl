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
    flat vec4   color;
    flat vec3   boundary_a;
    flat vec3   boundary_b;
    flat vec3   boundary_c;
} out_g;

layout(binding=0)   uniform usamplerBuffer  cells;
                    uniform mat3            NM;

layout(binding=1)   uniform samplerBuffer   field;
                    uniform float           solid_alpha;
                    uniform float           line_alpha;
                    uniform vec2            field_remap;
                    uniform bool            use_field;
                    uniform bool            log_map;
                    uniform bool            solid_pass;
                    uniform vec3            solid_color;

void
main()
{
    vec2 da = normalize( in_g[1].scr_pos - in_g[0].scr_pos );
    vec2 db = normalize( in_g[2].scr_pos - in_g[1].scr_pos );
    vec2 dc = normalize( in_g[0].scr_pos - in_g[2].scr_pos );

    uint cell = texelFetch( cells, gl_PrimitiveIDIn ).r;
    vec3 normal = normalize( NM*cross( in_g[1].obj_pos - in_g[0].obj_pos,
                                       in_g[2].obj_pos - in_g[0].obj_pos) );

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

    out_g.color = solid_alpha*vec4( max(0.2, normal.z)*color, 1.f);

    if( (cell & 0x40000000u) != 0u ) {
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

    if(  (cell & 0x10000000u) != 0u ) {
        out_g.boundary_c.xy = vec2( -dc.y, dc.x );
        out_g.boundary_c.z = -dot( out_g.boundary_c.xy, in_g[2].scr_pos );
    }
    else {
        out_g.boundary_c.xy = vec2( 0.f );
        out_g.boundary_c.z = 1.f;
    }

    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
}
