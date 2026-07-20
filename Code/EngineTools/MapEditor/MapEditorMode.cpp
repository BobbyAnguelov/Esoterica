#include "MapEditorMode.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    MapEditorMode::~MapEditorMode()
    {
        EE_ASSERT( m_pPropertyGrid == nullptr );
        EE_ASSERT( !m_prePropertyGridChangedEventID.IsValid() );
        EE_ASSERT( !m_postPropertyGridChangedEventID.IsValid() );
    }

    void MapEditorMode::Initialize( EditorContext* pEntityEditorContext )
    {
        const_cast<EditorContext*&>( m_pEntityEditorContext ) = pEntityEditorContext;
        const_cast<ToolsContext const*&>( m_pToolsContext ) = pEntityEditorContext->m_pToolsContext;

        m_pPropertyGrid = EE::New<PropertyGrid>( pEntityEditorContext->m_pToolsContext );
        m_prePropertyGridChangedEventID = m_pPropertyGrid->OnPreEdit().Bind( [this] ( PropertyEditInfo const& info ) { PrePropertyGridChange( info ); } );
        m_postPropertyGridChangedEventID = m_pPropertyGrid->OnPostEdit().Bind( [this] ( PropertyEditInfo const& info ) { PostPropertyGridChange( info ); } );
    }

    void MapEditorMode::Shutdown()
    {
        m_pPropertyGrid->OnPreEdit().Unbind( m_prePropertyGridChangedEventID );
        m_pPropertyGrid->OnPostEdit().Unbind( m_postPropertyGridChangedEventID );
        EE::Delete( m_pPropertyGrid );
    }
}