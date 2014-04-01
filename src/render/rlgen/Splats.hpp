#pragma once
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
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include "render/ManagedGL.hpp"

namespace render {

    namespace rlgen {

class Splats
        : public boost::noncopyable
{
public:
    
    Splats();
    
    ~Splats();
    
    /** Resize store to hold a given number of elements.
     *
     * \param elements Number of elements to hold.
     * \returns True if buffer contents were discarded.
     */
    bool
    resize( GLsizei elements );
    
    GLBuffer&
    buffer() { return m_compacted; }
    
    const GLVertexArrayObject&
    asAttributes() const { return  m_attributes; }
    
    GLQuery&
    query() { return m_query; }
    
    GLsizei
    count();
    
protected:
    GLsizei     m_alloc;
    GLBuffer    m_compacted;
    GLVertexArrayObject m_attributes;
    
    GLQuery     m_query;
    GLsizei     m_count;
    bool        m_update;
    
};


    } // of namespace rlgen
} // of namespace render
