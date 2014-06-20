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


#include "models/File.hpp"

namespace models {
    using std::string;
    static const string file_tab_key = "file_tab";
//    static const string file_tab_visible_key = "file_tab_visible";
    static const string file_name_key = "file_name";
    static const string file_load_key = "file_load";
    static const string file_refine_label_key = "file_refine";
    static const string file_refine_i_key = "file_refine_i";
    static const string file_refine_j_key = "file_refine_j";
    static const string file_refine_k_key = "file_refine_k";
    static const string file_preprocess_label_key = "file_preprocess";
    static const string file_triangulate_option_key = "file_triangulate";
//    static const std::string _key = "";


File::File( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic )
    : m_model( model ),
      m_logic( logic )
{
    m_model->addElement<bool>( file_tab_key, false, "Add source" );
//    m_model->addElement<bool>( file_tab_visible_key, false );
    m_model->addElement<string>( file_name_key, "" );
    m_model->addElement<bool>( file_load_key, false, "Load" );
    m_model->addElement<string>( file_refine_label_key, "", "Refinement" );
    m_model->addConstrainedElement<int>( file_refine_i_key, 1, 1, 16, "I" );
    m_model->addConstrainedElement<int>( file_refine_j_key, 1, 1, 16, "J" );
    m_model->addConstrainedElement<int>( file_refine_k_key, 1, 1, 16, "K" );
    m_model->addElement<string>( file_preprocess_label_key, "", "Preprocessing" );
    m_model->addElement<bool>( file_triangulate_option_key, false, "Triangulate" );

    m_model->addStateListener( file_load_key, this );
}

File::~File()
{
    m_model->removeStateListener( file_load_key, this );
}

void
File::setFileName( const std::string& filename )
{
    m_model->updateElement( file_name_key, filename );
}

const std::string&
File::titleKey() const
{
    return file_tab_key;
}

void
File::stateElementModified( tinia::model::StateElement * stateElement )
{
    const string& key = stateElement->getKey();
    if( key == file_load_key ) {
        bool value;
        stateElement->getValue( value );
        if( value ) {
            m_model->updateElement( file_load_key, false );
            m_model->updateElement( file_tab_key, false );

            string filename;
            m_model->getElementValue( file_name_key, filename );

            int refine_i, refine_j, refine_k;
            m_model->getElementValue( file_refine_i_key, refine_i );
            m_model->getElementValue( file_refine_j_key, refine_j );
            m_model->getElementValue( file_refine_k_key, refine_k );

            bool triangulate;
            m_model->getElementValue( file_triangulate_option_key, triangulate );

            m_logic.loadFile( filename, refine_i, refine_j, refine_k, triangulate );
        }
    }
}

tinia::model::gui::Element*
File::guiFactory() const
{
    using namespace tinia::model::gui;

    VerticalLayout* root = new VerticalLayout;
    ElementGroup* file_grp = new ElementGroup( file_tab_key );
    root->addChild( file_grp );
    VerticalLayout* file_layout0 = new VerticalLayout;
    file_grp->setChild( file_layout0 );
    file_layout0->addChild( new TextInput( file_name_key ) );
    HorizontalLayout* file_layout1 = new HorizontalLayout;
    file_layout0->addChild( file_layout1 );
    file_layout1->addChild( new Button( file_load_key ) );
    file_layout1->addChild( new HorizontalExpandingSpace );

    HorizontalLayout* wrap = new HorizontalLayout;
    root->addChild( wrap );

    ElementGroup* refine_grp = new ElementGroup( file_refine_label_key );
    wrap->addChild( refine_grp );
    Grid* refine_grid = new Grid( 2, 6 );
    refine_grp->setChild( refine_grid );
    refine_grid->setChild(0, 0, new Label( file_refine_i_key ) );
    refine_grid->setChild(0, 1, new SpinBox( file_refine_i_key ) );
    refine_grid->setChild(0, 2, new Label( file_refine_j_key ) );
    refine_grid->setChild(0, 3, new SpinBox( file_refine_j_key ) );
    refine_grid->setChild(0, 4, new Label( file_refine_k_key ) );
    refine_grid->setChild(0, 5, new SpinBox( file_refine_k_key ) );
    refine_grid->setChild(1, 0, new VerticalExpandingSpace );

    wrap->addChild( new HorizontalSpace );

    ElementGroup* pre_grp = new ElementGroup( file_preprocess_label_key );
    wrap->addChild( pre_grp );
    Grid* pre_grid = new Grid( 2, 1 );
    pre_grp->setChild( pre_grid );
    pre_grid->setChild( 0, 0, new CheckBox( file_triangulate_option_key ) );
    pre_grid->setChild( 1, 0, new VerticalExpandingSpace );

    return root;
}


} // of namespace models
