#version 330

in GO {
    flat uint   cell;
    vec3        normal;
} in_f;

layout(location=0) out uint cell;//vec4 frag_color;
layout(location=1) out vec3 normal;//vec4 frag_color;

void main(void)
{
    cell = in_f.cell;
    normal = in_f.normal;
}
