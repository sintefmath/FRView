#version 330
#extension GL_ARB_separate_shader_objects : enable
layout(points, invocations=1) in;
layout(points, max_vertices=1) out;

uniform usamplerBuffer  cell_subset;

in VG {
    uvec2 info;
    uvec3 indices;
} in_gs[];

layout(location=0) out uint cell;
layout(location=1) out uvec3 indices;

bool
selected( uint cell )
{
    if( cell == ~0u ) {
        return false;
    }
    cell = cell & 0x1fffffffu;
    uint s = ((texelFetch( cell_subset, int(cell/32u) ).r)>>(cell%32u))&0x1u;
    return s == 1u;
}


void
main()
{
    bvec2 inside = bvec2( selected( in_gs[0].info.x),
                          selected( in_gs[0].info.y ) );

    if( inside.y && !inside.x ) {
        cell    = in_gs[0].info.y;
        indices = in_gs[0].indices;
        EmitVertex();
    }
    else if( inside.x && !inside.y ) {
        cell    = in_gs[0].info.x | 0x80000000u;
        indices = in_gs[0].indices.zyx;
        EmitVertex();
    }
}
