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

#include <glm/glm.hpp>
#include "TimerQuery.hpp"

namespace render {

TimerQuery::TimerQuery()
    : m_has_data( false ), m_query(0) 
{
    reset();
}

TimerQuery::~TimerQuery()
{
    glDeleteQueries( 1, &m_query );
}

void
TimerQuery::reset()
{
    m_has_data = false;
    m_samples = 0;
    m_sample_sum = 0;
}

double
TimerQuery::samples()
{
    return m_samples;
}

double
TimerQuery::average()
{
    if( m_samples == 0 ) {
        return -1.f;
    }
    else {
        return glm::floor(static_cast<double>(m_sample_sum)/static_cast<double>(m_samples));
    }
}

void
TimerQuery::update()
{
    if(!m_has_data ) {
        return;
    }
    if( m_query == 0 ) {
        glGenQueries( 1, &m_query );
    }

    GLuint64 result;
    glGetQueryObjectui64v( m_query,
                           GL_QUERY_RESULT,
                           &result );
    m_sample_sum += result;
    m_samples++;
    m_has_data = false;
}

void
TimerQuery::beginQuery()
{
    if( m_query == 0 ) {
        glGenQueries( 1, &m_query );
    }
    glBeginQuery( GL_TIME_ELAPSED, m_query );
}

void
TimerQuery::endQuery()
{
    glEndQuery( GL_TIME_ELAPSED );
    m_has_data = true;
}

} // of namespace render
