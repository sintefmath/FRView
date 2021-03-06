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
#include <glm/glm.hpp>
#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>
#include "models/Logic.hpp"

namespace models {

class File
        : public tinia::model::StateListener
{
public:
    File( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic );

    ~File();

    void
    setFileName( const std::string& filename );

    const std::string&
    filename() const { return m_filename; }
    
    int
    refineI() const { return m_refine_i; }

    int
    refineJ() const { return m_refine_j; }

    int
    refineK() const { return m_refine_k; }
    
    bool
    triangulate() const { return m_triangulate; }
    
    const std::string&
    titleKey() const;

    const std::string&
    fileLoadKey() const;

    const std::string&
    fileNameKey() const;

    void
    stateElementModified( tinia::model::StateElement * stateElement );

    tinia::model::gui::Element*
    guiFactory() const;

protected:
    boost::shared_ptr<tinia::model::ExposedModel>   m_model;
    Logic&                                          m_logic;
    std::string                                     m_filename;
    int                                             m_refine_i;
    int                                             m_refine_j;
    int                                             m_refine_k;
    bool                                            m_triangulate;


};



} // of namespace models
