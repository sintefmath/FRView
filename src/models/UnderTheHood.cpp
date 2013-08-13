#include "models/UnderTheHood.hpp"

namespace models {
    using std::string;
    static const string under_the_hood_title_key = "under_the_hood_title";
    static const string profile_key = "profile";
    static const string profile_reset_key = "profile_reset";
    static const string profile_avg_fps_key = "profile_avg_fps";
    static const string profile_proxy_gen_key = "profile_proxy_gen";
    static const string profile_surface_gen_key = "profile_surface_gen";
    static const string profile_surface_render_key = "profile_surface_render";

UnderTheHood::UnderTheHood( boost::shared_ptr<tinia::model::ExposedModel>& model, Logic& logic )
    : m_model( model ),
      m_logic( logic ),
      m_profiling_enabled( false ),
      m_frames(0)
{
    m_model->addElement<bool>( under_the_hood_title_key, false, "Under the hood" );

    m_model->addElement<bool>( profile_key, m_profiling_enabled, "Enable profiling" );
    m_model->addElement<bool>( profile_reset_key, false, "Reset profiling counters" );
    m_model->addElement<string>( profile_avg_fps_key, "", "Average frame duration" );
    m_model->addElement<string>( profile_proxy_gen_key, "", "Create proxy" );
    m_model->addElement<string>( profile_surface_gen_key, "", "Create surface geo" );
    m_model->addElement<string>( profile_surface_render_key, "", "Render surface" );


    m_model->addStateListener( profile_key, this );
    m_model->addStateListener( profile_reset_key, this );
}

UnderTheHood::~UnderTheHood()
{
//    m_model->removeStateListener( profile_key, this );
//    m_model->removeStateListener( profile_reset_key, this );
}

const std::string&
UnderTheHood::titleKey() const
{
    return under_the_hood_title_key;
}

void
UnderTheHood::stateElementModified( tinia::model::StateElement * stateElement )
{
    const string& key = stateElement->getKey();
    if( key == profile_key ) {
        stateElement->getValue( m_profiling_enabled );
    }
    else if( key == profile_reset_key ) {
        bool value;
        stateElement->getValue( value );
        if( value ) {
            m_model->updateElement( profile_reset_key, false );
            m_proxy_gen.reset();
            m_surface_gen.reset();
            m_surface_render.reset();
            update( true );
        }
    }
}

tinia::model::gui::Element*
UnderTheHood::guiFactory() const
{
    using namespace tinia::model::gui;

    VerticalLayout* root = new VerticalLayout;

    ElementGroup* grp = new ElementGroup( under_the_hood_title_key );
    root->addChild( grp );
    VerticalLayout* vlayout = new VerticalLayout;
    grp->setChild( vlayout );

    vlayout->addChild( new tinia::model::gui::CheckBox( profile_key ) );
    Grid* grid = new Grid( 5, 2 );
    vlayout->addChild( grid );
    grid->setChild( 0, 0, new Button( profile_reset_key ) );
    grid->setChild( 1, 0, new Label( profile_avg_fps_key, false ) );
    grid->setChild( 1, 1, new Label( profile_avg_fps_key, true ) );
    grid->setChild( 2, 0, new Label( profile_proxy_gen_key, false ) );
    grid->setChild( 2, 1, new Label( profile_proxy_gen_key, true ) );
    grid->setChild( 3, 0, new Label( profile_surface_gen_key, false ) );
    grid->setChild( 3, 1, new Label( profile_surface_gen_key, true ) );
    grid->setChild( 4, 0, new Label( profile_surface_render_key, false ) );
    grid->setChild( 4, 1, new Label( profile_surface_render_key, true ) );

    return root;
}

void
UnderTheHood::update( bool force )
{
    if( !force && !m_profiling_enabled ) {
        return;
    }
    m_proxy_gen.update();
    m_surface_gen.update();
    m_surface_render.update();
    PerfTimer curr;
    if(!force) {
        m_frames++;
    }

    double delta = PerfTimer::delta( m_update_timer, curr );
    if( force || delta > 0.5 ) {
        std::stringstream o;
        o.str("");
        o << std::setprecision(4 )
          << ((1e3*delta)/m_frames) << " ms ("
          << std::setprecision(5 )
          << (m_frames/delta ) << " fps)";
        //("
        //  << m_proxy_gen.samples() << " samples)";
        m_model->updateElement<string>( profile_avg_fps_key, o.str() );

        m_frames = 0;
        m_update_timer.reset();

        o.str("");
        o << std::setprecision(4 )
          << (m_proxy_gen.average()*1e-6 ) << " ms ("
          << m_proxy_gen.samples() << " samples)";
        m_model->updateElement<string>( profile_proxy_gen_key, o.str() );

        o.str("");
        o << std::setprecision(4 )
          << (m_surface_gen.average()*1e-6 ) << " ms ("
          << m_surface_gen.samples() << " samples)";
        m_model->updateElement<string>( profile_surface_gen_key, o.str() );

        o.str("");
        o << std::setprecision(4 )
          << (m_surface_render.average()*1e-6 ) << " ms ("
          << m_surface_render.samples() << " samples)";
        m_model->updateElement<string>( profile_surface_render_key, o.str() );

    }
}



} // of namespace models
