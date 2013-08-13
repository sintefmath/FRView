#ifdef GL_ES
precision highp float;
#endif
varying vec3 color;
varying vec3 normal;
void
main()
{
    float d = max( 0.3, dot( normalize(vec3( 0.0, 1.5, 1.5 )), normal ) );
    gl_FragColor = vec4( d*color, 1.0 );
}
