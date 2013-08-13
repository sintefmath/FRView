#version 330
layout(triangles, invocations=1) in;
layout(triangle_strip, max_vertices=3) out;

out GO {
    flat uint   cell;
    vec3        normal;
} out_g;

uniform mat4 MVP;
uniform mat3 NM;



#ifdef COMPACT
// -----------------------------------------------------------------------------
uniform usamplerBuffer cells;

void
main()
{
    uint cell = texelFetch( cells, gl_PrimitiveIDIn ).r;
    out_g.cell = cell;

    vec3 a = (1.f/gl_in[0].gl_Position.w)*gl_in[0].gl_Position.xyz;
    vec3 b = (1.f/gl_in[1].gl_Position.w)*gl_in[1].gl_Position.xyz;
    vec3 c = (1.f/gl_in[2].gl_Position.w)*gl_in[2].gl_Position.xyz;

    vec3 normal = vec3(0);//normalize( NM*cross( b-a, c-a) );

    uint dir = cell>>29u;
    if( dir == 0u ) {
        normal = vec3(1,0,0);
    }
    else if( dir == 1u ) {
        normal = vec3(0,-1,0);
    }
    else if( dir == 2u ) {
        normal = vec3(0,0,-1);
    }
    else if( dir == 4u ) {
        normal = vec3(-1,0,0);
    }
    else if( dir == 5u ) {
        normal = vec3(0,1,0);
    }
    else if( dir == 6u ) {
        normal = vec3(0,0,1);
    }

    normal = normalize( NM*normal );
    normal = normalize( NM*cross( b-a, c-a) );

    out_g.normal = normal;

    gl_Position = MVP*gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = MVP*gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = MVP*gl_in[2].gl_Position;
    EmitVertex();
}

// -----------------------------------------------------------------------------
#else
// -----------------------------------------------------------------------------

uniform usamplerBuffer  tri_info;
uniform usamplerBuffer  cell_subset;

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
    uvec2 cells = texelFetch( tri_info, gl_PrimitiveIDIn ).xy;

    bvec2 inside = bvec2( selected( cells.x),
                          selected( cells.y ) );

    if( inside.y && !inside.x ) {
        vec3 a = (1.f/gl_in[0].gl_Position.w)*gl_in[0].gl_Position.xyz;
        vec3 b = (1.f/gl_in[1].gl_Position.w)*gl_in[1].gl_Position.xyz;
        vec3 c = (1.f/gl_in[2].gl_Position.w)*gl_in[2].gl_Position.xyz;

        vec3 normal = normalize( NM*cross( b-a, c-a) );

        out_g.cell = cells.y;
        out_g.normal = normal;

        gl_Position = MVP*gl_in[0].gl_Position;
        EmitVertex();
        gl_Position = MVP*gl_in[1].gl_Position;
        EmitVertex();
        gl_Position = MVP*gl_in[2].gl_Position;
        EmitVertex();
    }
    else if( inside.x && !inside.y ) {
        vec3 a = (1.f/gl_in[2].gl_Position.w)*gl_in[2].gl_Position.xyz;
        vec3 b = (1.f/gl_in[1].gl_Position.w)*gl_in[1].gl_Position.xyz;
        vec3 c = (1.f/gl_in[0].gl_Position.w)*gl_in[0].gl_Position.xyz;

        vec3 normal = normalize(NM* cross( b-a, c-a) );

        out_g.cell = cells.x;
        out_g.normal = normal;

        gl_Position = MVP*gl_in[2].gl_Position;
        EmitVertex();
        gl_Position = MVP*gl_in[1].gl_Position;
        EmitVertex();
        gl_Position = MVP*gl_in[0].gl_Position;
        EmitVertex();
    }
}
// -----------------------------------------------------------------------------
#endif
