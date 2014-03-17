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

#pragma once
#include <memory>
#include <boost/shared_ptr.hpp>
#include <GL/glew.h>
#include <string>
#include "render/ManagedGL.hpp"
#include "bridge/FieldBridge.hpp"

namespace render {
    namespace mesh {
        class CellSetInterface;
    }

/** Represent a single property at a single report step for all cells in a grid. */
class GridField
{
    friend class GridFieldBridge;
public:
    GridField( boost::shared_ptr<mesh::CellSetInterface> grid );

    /** Get texture buffer sampling the field (indexed by local cell indices). */
    GLuint
    texture() const
    { return m_texture.get(); }

    /** Get minimum value of property over all cells. */
    const float
    minValue() const
    { return m_min_value; }

    /** Get maximum value of property over all cells. */
    const float
    maxValue() const
    { return m_max_value; }

    /** True if object is populated with any data. */
    bool
    hasData() const { return m_has_data; }

    void
    import( bridge::FieldBridge& bridge );

protected:
    boost::shared_ptr<mesh::CellSetInterface>   m_cell_set;
    bool                        m_has_data;
    GLBuffer                    m_buffer;
    GLTexture                   m_texture;
    GLsizei                     m_count;
    float                       m_min_value;
    float                       m_max_value;
};

} // of namespace render
