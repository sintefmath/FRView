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

namespace models {

class SourceSelector
        : public tinia::model::StateListener
{
public:
    typedef int Revision;

    SourceSelector( boost::shared_ptr<tinia::model::ExposedModel>& model );
    
    Revision
    revision() const { return m_revision; }

    void
    stateElementModified( tinia::model::StateElement * stateElement );

    tinia::model::gui::Element*
    guiFactory() const;
    
    void
    updateSources( std::vector<std::string>& sources );
    
protected:
    boost::shared_ptr<tinia::model::ExposedModel>   m_model;
    Revision                                        m_revision;

    void
    bumpRevision();
};

} // of namespace models
    