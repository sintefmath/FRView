#pragma once
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

#include <memory>
#include <glm/glm.hpp>
#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>
#include "models/Logic.hpp"
#include "models/File.hpp"

namespace models {

class SourceSelector
        : public tinia::model::StateListener
{
public:
    typedef int Revision;

    SourceSelector( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic  );
    
    Revision
    revision() const { return m_revision; }

    void
    stateElementModified( tinia::model::StateElement * stateElement );

    const std::string&
    getSourceSelectorKey() const;

    const std::string&
    getCloneKey() const;

    const std::string&
    getDeleteKey() const;

    tinia::model::gui::Element*
    guiFactory() const;
    
    void
    updateSources( std::vector<std::string>& sources );
    
    File&
    file() { return m_file; }

    const File&
    file() const { return m_file; }

protected:
    boost::shared_ptr<tinia::model::ExposedModel>   m_model;
    Logic&                                          m_logic;
    models::File                                    m_file;
    std::vector<std::string>                        m_sources;
    Revision                                        m_revision;

    void
    bumpRevision();
};

} // of namespace models
    
