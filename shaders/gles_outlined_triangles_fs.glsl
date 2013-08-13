#ifdef GL_ES
precision highp float;
#endif
varying vec3 barycentric;
void
main()
{
    float d = min( barycentric.x, min( barycentric.y, barycentric.z) );
    float l = max(0.0, 1.0 - 5.0*d);
    gl_FragColor = vec4( 0.2+l, 0.2+l, 0.5+l, 1.0 );
}
