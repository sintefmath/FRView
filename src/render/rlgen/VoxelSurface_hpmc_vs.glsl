#version 330
/* Copyright STIFTELSEN SINTEF 2013
 *
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */


uniform mat4 mvp;

// Forward declaration of func provided by HPMC
void
extractVertex( out vec3 a, out vec3 b, out vec3 p, out vec3 n );

out vec4 position;

uniform vec3            scale;
uniform vec3            shift;
uniform bool            use_field;
uniform vec2            field_remap;
uniform samplerBuffer   field;
uniform usampler3D      voxels;
#define VOXEL_SAMPLER_DEFINED

void
main()
{
    vec3 a, b, p, n;
    extractVertex( a, b, p, n );

    // see voxel_surface_extractor_fetch.glsl for texture definition
    float value = 0.5;
    if( use_field ) {
        uint cell = (max( texture( voxels, a ).r, texture( voxels, b).r )-1u) & 0x0fffffffu;
        float v = texelFetch( field, int(cell) ).r;
        value = clamp( field_remap.y*( v - field_remap.x), 0.0, 1.0 );
    }


    vec3 an = abs( n );
    float s = 1.f/max( max( an.x, an.y ), an.z );   // taxicab-norm-scale

    vec3 ni = clamp( vec3(round(s*n)+vec3(2)), vec3(1.0), vec3(3.0));                       // shift zero to 2 (range should be [1,3])

    position = vec4(clamp(scale*p+shift, vec3(0.0), vec3(0.9999)) + floor(ni), value );
}
