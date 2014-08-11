#version 420
/* Copyright STIFTELSEN SINTEF 2014
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
 * along with the FRView.  If not, see http://www.gnu.org/licenses/.
 */
layout(points, invocations=1) in;
layout(points, max_vertices=MAX_OUT) out;


layout(location=0)  out uvec4               cell;
layout(location=1)  out uvec3               indices;

void
emit_triangle( in uint cell_ix, in uvec3 nrm_ix, in uvec3 vtx_ix )
{
    cell    = uvec4( cell_ix, nrm_ix );
    indices = vtx_ix;
    EmitVertex();
}

// void main() etc. from common triangulate_gs.glsl
