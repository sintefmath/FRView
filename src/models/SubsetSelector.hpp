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

#include <tinia/model/GUILayout.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/model/ExposedModel.hpp>
#include "job/SourceItem.hpp"
#include "models/Logic.hpp"



namespace models {

class SubsetSelectorData
{
public:    
    SubsetSelectorData();

protected:
    
};

class SubsetSelector
        : public tinia::model::StateListener
{
public:
    typedef int Revision;
    

    SubsetSelector( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic  );
    
    Revision
    revision() const { return m_revision; }

    void
    stateElementModified( tinia::model::StateElement * stateElement );

    tinia::model::gui::Element*
    guiFactory() const;

    void
    update( boost::shared_ptr<SourceItem> source_item );

protected:
    boost::shared_ptr<tinia::model::ExposedModel>   m_model;
    Logic&                                          m_logic;
    boost::shared_ptr<SourceItem>                   m_source_item;
    Revision                                        m_revision;

    void
    bumpRevision(); 
};


} // of namespace models