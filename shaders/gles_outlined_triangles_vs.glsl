uniform mat4 MVP;
attribute vec3 position;
varying vec3 barycentric;
void
main()
{
    vec3 p = position;
    int i = int( floor(p.x) );
    p.x = fract( p.x );

    barycentric = vec3( i == 0, i==1, i==2 );
    gl_Position = MVP * vec4( p, 1.0 );
}
