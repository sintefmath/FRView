#ifdef GL_ES
precision highp float;
#endif
uniform mat4 MVP;
uniform mat3 NM;
attribute vec4 position;
varying vec3 normal;
varying vec3 color;
void
main()
{
    vec3 n = floor( position.xyz )-vec3( 2.0 );
    vec3 p = fract( position.xyz );
    normal = NM * n;
    float scalar = position.w;
    color = max( vec3( 0.0 ),
                     sin( vec3( 4.14*(scalar - 0.5),
                                3.14*(scalar),
                                4.14*(scalar + 0.5) ) ) );
    gl_Position = MVP * vec4( p, 1.0 );
}
