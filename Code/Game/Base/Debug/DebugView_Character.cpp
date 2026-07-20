#include "DebugView_Character.h"
#include "Game/Base/Components/Component_Character.h"
#include "Game/GameFlow/Systems/WorldSystem_GameFlow.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    void CharacterDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pGameFlowManager = pWorld->GetWorldSystem<GameFlowManager>();

        m_windows.emplace_back( "Characters", [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t ) { DrawActorDebugView( context, isFocused ); } );
    }

    void CharacterDebugView::Shutdown()
    {
        m_pGameFlowManager = nullptr;
        DebugView::Shutdown();
    }

    void CharacterDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        if ( ImGui::MenuItem( "Characters" ) )
        {
            m_windows[0].m_isOpen = true;
        }
    }

    void CharacterDebugView::Update( EntityWorldUpdateContext const& context )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( m_pGameFlowManager != nullptr );

        if ( m_windows[0].m_isOpen )
        {
            EE_ASSERT( m_debuggedCharacterIDs.size() == m_debuggedCharacters.size() );
            EE_ASSERT( m_debuggedCharacterIDs.size() == m_openTabs.size() );
            for ( size_t i = 0; i < m_debuggedCharacterIDs.size(); i++ )
            {
                if ( !m_openTabs[i] || !m_pGameFlowManager->GetCharacters().HasItemForID( m_debuggedCharacterIDs[i] ) )
                {
                    m_debuggedCharacterIDs.erase( m_debuggedCharacterIDs.begin() + i );
                    m_debuggedCharacters.erase( m_debuggedCharacters.begin() + i );
                    m_openTabs.erase( m_openTabs.begin() + i );
                    i--;
                }
            }
        }
    }

    void CharacterDebugView::DrawActorDebugView( EntityWorldUpdateContext const& context, bool isFocused )
    {
        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::BeginCombo( "##ActorPicker", EE_ICON_BUG" Pick Actors To Debug", ImGuiComboFlags_HeightLargest ) )
        {
            TIDVector<ComponentID, CharacterComponent*> const& characters = m_pGameFlowManager->GetCharacters();

            ImGuiListClipper clipper;
            clipper.Begin( (int32_t) characters.size() );
            while ( clipper.Step() )
            {
                for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                {
                    auto pCharacter = characters[i];
                    bool isDebugged = VectorContains( m_debuggedCharacterIDs, pCharacter->GetID() );

                    InlineString str( InlineString::CtorSprintf(), "%s (%llu)", pCharacter->GetNameID().c_str(), pCharacter->GetID().m_value );
                    if ( ImGui::Checkbox( str.c_str(), &isDebugged ) )
                    {
                        if ( isDebugged )
                        {
                            m_debuggedCharacterIDs.emplace_back( pCharacter->GetID() );
                            m_debuggedCharacters.emplace_back( pCharacter );
                            m_openTabs.emplace_back( true );
                        }
                        else
                        {
                            m_openTabs[i] = false;
                        }
                    }
                }
            }

            ImGui::EndCombo();
        }

        //-------------------------------------------------------------------------

        if ( !m_openTabs.empty() )
        {
            if ( ImGui::BeginTabBar( "Actors" ) )
            {
                for ( size_t i = 0; i < m_debuggedCharacterIDs.size(); i++ )
                {
                    InlineString str( InlineString::CtorSprintf(), "%s##%llu", m_debuggedCharacters[i]->GetNameID().c_str(), m_debuggedCharacters[i]->GetID().m_value );
                    if ( ImGui::BeginTabItem( str.c_str(), &m_openTabs[i] ) )
                    {
                        DrawActorDebugTab( context, isFocused, m_debuggedCharacters[i] );
                        ImGui::EndTabItem();
                    }
                }

                ImGui::EndTabBar();
            }
        }
    }

    void CharacterDebugView::DrawActorDebugTab( EntityWorldUpdateContext const& context, bool isFocused, CharacterComponent* pCharacter )
    {
        ImGui::SeparatorText( "Shape" );
        ImGui::Checkbox( "Debug Shape", &pCharacter->m_debugCapsule );

        if ( pCharacter->m_debugCapsule )
        {
            ImGui::Text( "Before: %s", Math::ToString( pCharacter->m_debugPreMoveTransform.GetTranslation() ).c_str() );
            ImGui::Text( "After: %s", Math::ToString( pCharacter->m_debugPostMoveTransform.GetTranslation() ).c_str() );
            ImGui::Text( "Delta: %s", Math::ToString( pCharacter->m_debugPostMoveTransform.GetTranslation() - pCharacter->m_debugPreMoveTransform.GetTranslation() ).c_str() );
        }

        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Info" );

        ImGui::Text( "Velocity: %.3f, %.3f, %.3f", pCharacter->m_velocity.GetX(), pCharacter->m_velocity.GetY(), pCharacter->m_velocity.GetZ() );
        ImGui::Text( "Speed: %.3f", pCharacter->m_velocity.GetLength3() );

        switch ( pCharacter->m_floorType )
        {
            case FloorType::Floor:
            {
                ImGuiX::ScopedFont sf( Colors::Green );
                ImGui::Text( "Floor Type: Navigable" );
            }
            break;

            case FloorType::Slope:
            {
                ImGuiX::ScopedFont sf( Colors::Red );
                ImGui::Text( "Floor Type: Unnavigable" );
            }
            break;

            case FloorType::None:
            {
                ImGui::Text( "Floor Type: None" );
            }
            break;
        }

        ImGui::Text( "Time without floor: %.2fs", pCharacter->m_timeWithoutFloor.GetElapsedTimeSeconds().ToFloat() );
        ImGui::Checkbox( "Draw floor info", &pCharacter->m_debugFloor );
    }

    void CharacterDebugView::DrawOverlayUI( EntityWorldUpdateContext const& context )
    {
        EE_ASSERT( m_pWorld != nullptr );
        EE_ASSERT( m_pGameFlowManager != nullptr );

        auto drawingCtx = context.GetDebugDrawContext();

        for ( size_t i = 0; i < m_debuggedCharacterIDs.size(); i++ )
        {
            auto pCharacter = m_debuggedCharacters[i];
            float const radius = pCharacter->GetCapsuleRadius();
            float const halfHeight = pCharacter->GetCapsuleHalfHeight();

            if ( pCharacter->m_debugCapsule )
            {
                drawingCtx.DrawWireCapsule( pCharacter->m_debugPreMoveTransform, radius, halfHeight, Colors::White, 2.0f );
                drawingCtx.DrawWireCapsule( pCharacter->m_debugPostMoveTransform, radius, halfHeight, Colors::Green, 2.0f );
            }

            //-------------------------------------------------------------------------

            if ( pCharacter->m_debugFloor )
            {
                Vector const floorPosition = pCharacter->GetCapsuleBottom();
                drawingCtx.DrawPoint( floorPosition, Colors::Yellow );
                drawingCtx.DrawArrow( floorPosition, floorPosition + ( pCharacter->m_floorNormal * 0.5f ), Colors::Yellow );
                drawingCtx.DrawCircle( floorPosition, Axis::Z, radius, Colors::Yellow );
            }
        }
    }
}
#endif