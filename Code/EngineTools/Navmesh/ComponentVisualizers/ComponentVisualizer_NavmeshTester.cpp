#include "ComponentVisualizer_NavmeshTester.h"
#include "Engine/Navmesh/Components/Component_NavmeshTester.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Drawing/DebugDrawingSystem.h"
#include "Base/Drawing/DebugDrawing.h"
#include "imgui.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    TypeSystem::TypeID NavmeshTesterComponentVisualizer::GetSupportedType() const
    {
        return NavmeshTesterComponent::GetStaticTypeID();
    }

    void NavmeshTesterComponentVisualizer::UpdateVisualizedComponent( EntityComponent* pComponent )
    {
        ComponentVisualizer::UpdateVisualizedComponent( pComponent );
        m_pTesterComponent = Cast<NavmeshTesterComponent>( pComponent );
        m_startTransform = m_pTesterComponent->m_startTransform;
        m_endTransform = m_pTesterComponent->m_endTransform;
    }

    void NavmeshTesterComponentVisualizer::DrawToolbar( ComponentVisualizerContext const& context )
    {
        ImGui::MenuItem( "Das " );
    }

    void NavmeshTesterComponentVisualizer::Visualize( ComponentVisualizerContext const& context )
    {
        auto const startGizmoResult = m_startGizmo.Draw( m_startTransform.GetTranslation(), m_startTransform.GetRotation(), *context.m_pWorld->GetViewport() );
        switch ( startGizmoResult.m_state )
        {
            case ImGuiX::Gizmo::State::StartedManipulating:
            {
                m_startTransform = startGizmoResult.GetModifiedTransform( m_startTransform );
            }
            break;

            case ImGuiX::Gizmo::State::Manipulating:
            {
                m_startTransform = startGizmoResult.GetModifiedTransform( m_startTransform );
            }
            break;

            case ImGuiX::Gizmo::State::StoppedManipulating:
            {
                m_startTransform = startGizmoResult.GetModifiedTransform( m_startTransform );
                context.m_preComponentEditFunction( m_pComponent );
                m_pTesterComponent->m_startTransform = m_startTransform;
                context.m_postComponentEditFunction( m_pComponent );
            }
            break;

            default:
            break;
        }

        //-------------------------------------------------------------------------

        //if ( startGizmoResult.m_state != ImGuiX::Gizmo::State::Manipulating )
        {
            /*auto const endGizmoResult = m_endGizmo.Draw( m_endTransform.GetTranslation(), m_endTransform.GetRotation(), *context.m_pWorld->GetViewport() );
            switch ( endGizmoResult.m_state )
            {
                case ImGuiX::Gizmo::State::StartedManipulating:
                {
                    m_endTransform = startGizmoResult.GetModifiedTransform( m_endTransform );
                }
                break;

                case ImGuiX::Gizmo::State::Manipulating:
                {
                    m_endTransform = startGizmoResult.GetModifiedTransform( m_endTransform );
                }
                break;

                case ImGuiX::Gizmo::State::StoppedManipulating:
                {
                    context.m_preComponentEditFunction( m_pComponent );
                    m_pTesterComponent->m_endTransform = m_endTransform;
                    context.m_postComponentEditFunction( m_pComponent );
                }
                break;

                default:
                break;
            }*/
        }
    }
}