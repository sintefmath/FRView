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
 * along with the FRView.  If not, see http://www.gnu.org/licenses/.
 */

layout(location=0) in uvec2 cell_ix;
layout(location=1) in uint  offset_a;
layout(location=2) in uint  offset_b;

out VG {
    uvec2 cell_ix;
    uint  offset_a;
    uint  offset_b;
} to_gs;


void
main()
{
    to_gs.cell_ix = cell_ix;
    to_gs.offset_a = offset_a;
    to_gs.offset_b = offset_b;
}
