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

#include <string>
#include "models/SourceSelector.hpp"

namespace {
    using std::string;

    const string source_selector_key = "source_selector";
    const string source_clone_key = "source_selector_clone";
    const string source_delete_key = "source_selector_delete";

    const char* source_selector_none[1] = { "None" };



} // of anonymous namespace

namespace models {

SourceSelector::SourceSelector(boost::shared_ptr<tinia::model::ExposedModel>& model , Logic &logic)
    : m_model( model ),
      m_logic( logic ),
      m_revision( 1 )
{
    m_model->addElementWithRestriction<std::string>( source_selector_key,
                                                     source_selector_none[ 0 ],
                                                    &source_selector_none[0],
                                                    &source_selector_none[1] );
    m_model->addStateListener( source_selector_key, this );

    m_model->addElement<bool>( source_clone_key, false, "Clone" );
    m_model->addStateListener( source_clone_key, this );

    m_model->addElement<bool>( source_delete_key, false, "Delete" );
    m_model->addStateListener( source_delete_key, this );
}

void
SourceSelector::updateSources( std::vector<std::string>& sources )
{
    m_sources = sources;
    if( m_sources.empty() ) {
        // --- No sources ------------------------------------------------------
        m_model->updateRestrictions<std::string>( source_selector_key,
                                     source_selector_none[ 0 ],
                                    &source_selector_none[0],
                                    &source_selector_none[1] );
    }
    else {
        // --- One or more sources, get current selection ----------------------
        std::string current;
        m_model->getElementValue( source_selector_key, current );
        
        for( size_t i=0; i<m_sources.size(); i++ ) {
            // --- found current selection, adjust index -----------------------
            if( m_sources[i] == current ) {
                m_model->updateRestrictions( source_selector_key,
                                             m_sources[i],
                                             m_sources.begin(),
                                             m_sources.end() );
                m_logic.setSource( i );
                return;
            }
        }
        // --- current selection not in new list, use first element ------------
        m_model->updateRestrictions( source_selector_key,
                                     m_sources[0],
                                     m_sources.begin(),
                                     m_sources.end() );
        m_logic.setSource( 0 );
    }
}


void
SourceSelector::bumpRevision()
{
}

void
SourceSelector::stateElementModified( tinia::model::StateElement * stateElement )
{
    const std::string& key = stateElement->getKey();
    if( key == source_selector_key ) {
        std::string value;
        stateElement->getValue( value );
        
        for(size_t i=0; i<m_sources.size(); i++ ) {
            if( m_sources[i] == value ) {
                m_logic.setSource( i );
                return;
            }
        }
    }

    else if( key == source_clone_key ) {
        bool value;
        stateElement->getValue<bool>( value );
        if( value ) {
            m_model->updateElement( key, false );
            m_logic.cloneSource();
            return;
        }
    }

    else if( key == source_delete_key ) {
        bool value;
        stateElement->getValue<bool>( value );
        if( value ) {
            m_model->updateElement( key, false );
            m_logic.deleteSource();
        }
    }

}

tinia::model::gui::Element*
SourceSelector::guiFactory() const
{
    using namespace tinia::model::gui;

    VerticalLayout* vl = new VerticalLayout;
    vl->addChild( new ComboBox( source_selector_key ) );

    HorizontalLayout* hl = new HorizontalLayout;
    hl->addChild( new Button( source_clone_key ) );
    hl->addChild( new HorizontalExpandingSpace );
    hl->addChild( new Button( source_delete_key ) );
    vl->addChild( hl );

    return vl;
}

} // of namespace models
