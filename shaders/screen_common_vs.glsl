#version 330
layout(location=0) in vec4 position;

out vec2 normalized;

void
main()
{
    normalized = 0.5f*(position.xy + vec2(1.f) );
    gl_Position = position;
}
