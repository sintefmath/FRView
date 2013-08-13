#version 330
#extension GL_ARB_texture_gather : enable

in vec2 normalized;

layout(location=0) out vec3 frag_color;

#ifdef FIELD
uniform samplerBuffer   field;
uniform vec2            linear_map;
#endif
uniform usampler2D      cell_index;
uniform sampler2D       normal;
uniform sampler2D       depth;
uniform bool            wireframe;

void main(void)
{
    ivec2 fc = ivec2( gl_FragCoord.xy );
    uvec4 cells = textureGather( cell_index, normalized );
    vec4 depths = textureGather( depth, normalized );



    float line = 0.f;
    float depth = depths.x;
    float ldepth = min( min( depths.x, depths.y ), depths.w );

    if( wireframe ) {

        if( cells.x != cells.y ) {
            line = 1.0f;
            depth = ldepth;
        }
        if( cells.x != cells.w ) {
            line = 1.0f;
            depth = ldepth;
        }

    }

    /*
    if( wireframe ) {
        if( cells.x != cells.y ) {
            line += 0.25f;
        }
        if( cells.z != cells.w ) {
            line += 0.25f;
        }
        if( cells.x != cells.w ) {
            line += 0.25;
        }
        if( cells.y != cells.z ) {
            line += 0.25;
        }
    }
*/



    uint cell = cells.x & 0x1fffffffu;
    vec3 n = texelFetch( normal, ivec2( gl_FragCoord.xy ), 0 ).rgb;
#ifdef FIELD
    float scalar = linear_map.y*(texelFetch( field, int(cell) ).r - linear_map.x);
    vec3 diffuse = max( vec3(0.f),
                        sin( vec3( 4.14f*(scalar - 0.5f),
                                   3.14f*(scalar),
                                   4.14f*(scalar + 0.5f) ) ) );
#else
    vec3 diffuse = vec3(  0.2*float( int( cell & 8u  ) == 0 ) + 0.2*float( int( cell & 1u ) == 0  ) + 0.2,
                          0.2*float( int( cell & 16u ) == 0 ) + 0.2*float( int( cell & 2u ) == 0  ) + 0.2,
                          0.2*float( int( cell & 32u ) == 0 ) + 0.2*float( int( cell & 4u ) == 0  ) + 0.2 );
#endif
    frag_color = max(0.0,n.z)*diffuse + vec3(line);
    gl_FragDepth = depth;
}
