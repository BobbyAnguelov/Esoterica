#include "DebugView_Physics.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/Components/Component_PhysicsCapsule.h"
#include "Engine/Physics/Components/Component_PhysicsSphere.h"
#include "Engine/Physics/Components/Component_PhysicsBox.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/Imgui/ImguiX.h"

#include <PxVisualizationParameter.h>

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS

using namespace physx;

//-------------------------------------------------------------------------

namespace EE::Physics
{
    bool PhysicsDebugView::DrawMaterialDatabaseView( UpdateContext const& context )
    {
        auto pPhysicsSystem = context.GetSystem<PhysicsSystem>();

        //-------------------------------------------------------------------------

        bool isMaterialDatabaseWindowOpen = true;

        ImGui::SetNextWindowBgAlpha( 0.75f );
        if ( ImGui::Begin( "Physics Material Database", &isMaterialDatabaseWindowOpen ) )
        {
            if ( ImGui::BeginTable( "Resource Request History Table", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
            {
                ImGui::TableSetupColumn( "Material ID", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Static Friction", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 96 );
                ImGui::TableSetupColumn( "Dynamic Friction", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 96 );
                ImGui::TableSetupColumn( "Restitution", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 96 );
                ImGui::TableSetupColumn( "Friction Combine", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 128 );
                ImGui::TableSetupColumn( "Restitution Combine", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 128 );

                //-------------------------------------------------------------------------

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                for ( auto const& materialPair : pPhysicsSystem->m_materials )
                {
                    PxMaterial const* pMaterial = materialPair.second.m_pMaterial;

                    ImGui::TableNextRow();

                    //-------------------------------------------------------------------------

                    static char const* const combineText[] = { "Average", "Min", "Multiply", "Max" };
                    auto GetCombineText = [] ( PxCombineMode::Enum combineMode )
                    {
                        return combineText[(uint8_t) combineMode];
                    };

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Text( materialPair.second.m_ID.c_str() );

                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::Text( "%.3f", pMaterial->getStaticFriction() );

                    ImGui::TableSetColumnIndex( 2 );
                    ImGui::Text( "%.3f", pMaterial->getDynamicFriction() );

                    ImGui::TableSetColumnIndex( 3 );
                    ImGui::Text( "%.3f", pMaterial->getRestitution() );

                    ImGui::TableSetColumnIndex( 4 );
                    ImGui::Text( GetCombineText( pMaterial->getFrictionCombineMode() ) );

                    ImGui::TableSetColumnIndex( 5 );
                    ImGui::Text( GetCombineText( pMaterial->getRestitutionCombineMode() ) );
                }

                //-------------------------------------------------------------------------

                ImGui::EndTable();
            }
        }
        ImGui::End();

        //-------------------------------------------------------------------------

        return isMaterialDatabaseWindowOpen;
    }

    //-------------------------------------------------------------------------

    PhysicsDebugView::PhysicsDebugView()
    {
        m_menus.emplace_back( DebugMenu( "Engine/Physics", [this] ( EntityWorldUpdateContext const& context ) { DrawPhysicsMenu( context ); } ) );
    }

    void PhysicsDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        m_pPhysicsSystem = systemRegistry.GetSystem<PhysicsSystem>();
        m_pPhysicsWorldSystem = pWorld->GetWorldSystem<PhysicsWorldSystem>();
    }

    void PhysicsDebugView::Shutdown()
    {
        m_pPhysicsSystem = nullptr;
        m_pPhysicsWorldSystem = nullptr;
    }

    //-------------------------------------------------------------------------

    void PhysicsDebugView::DrawPhysicsMenu( EntityWorldUpdateContext const& context )
    {
        //-------------------------------------------------------------------------
        // Scene
        //-------------------------------------------------------------------------

        uint32_t debugFlags = m_pPhysicsWorldSystem->GetDebugFlags();
        float drawDistance = m_pPhysicsWorldSystem->GetDebugDrawDistance();

        bool stateUpdated = false;

        stateUpdated |= ImGui::CheckboxFlags( "Enable Debug Visualization", &debugFlags, 1 << PxVisualizationParameter::eSCALE );
        stateUpdated |= ImGui::SliderFloat( "Visualization Distance", &drawDistance, 1.0f, 100.0f );

        DrawPVDMenu( context );

        ImGui::Separator();

        stateUpdated |= ImGui::CheckboxFlags( "Collision AABBs", &debugFlags, 1 << PxVisualizationParameter::eCOLLISION_AABBS );
        stateUpdated |= ImGui::CheckboxFlags( "Collision Shapes", &debugFlags, 1 << PxVisualizationParameter::eCOLLISION_SHAPES );
        stateUpdated |= ImGui::CheckboxFlags( "Collision Axes", &debugFlags, 1 << PxVisualizationParameter::eCOLLISION_AXES );
        stateUpdated |= ImGui::CheckboxFlags( "Collision Face Normals", &debugFlags, 1 << PxVisualizationParameter::eCOLLISION_FNORMALS );
        stateUpdated |= ImGui::CheckboxFlags( "Collision Edges", &debugFlags, 1 << PxVisualizationParameter::eCOLLISION_EDGES );

        ImGui::Separator();

        stateUpdated |= ImGui::CheckboxFlags( "Contact Points", &debugFlags, 1 << PxVisualizationParameter::eCONTACT_POINT );
        stateUpdated |= ImGui::CheckboxFlags( "Contact Normals", &debugFlags, 1 << PxVisualizationParameter::eCONTACT_NORMAL );
        stateUpdated |= ImGui::CheckboxFlags( "Contact Error", &debugFlags, 1 << PxVisualizationParameter::eCONTACT_ERROR );
        stateUpdated |= ImGui::CheckboxFlags( "Contact Force", &debugFlags, 1 << PxVisualizationParameter::eCONTACT_FORCE );

        ImGui::Separator();

        stateUpdated |= ImGui::CheckboxFlags( "Actor Axes", &debugFlags, 1 << PxVisualizationParameter::eACTOR_AXES );
        stateUpdated |= ImGui::CheckboxFlags( "Body Axes", &debugFlags, 1 << PxVisualizationParameter::eBODY_AXES );
        stateUpdated |= ImGui::CheckboxFlags( "Linear Velocity", &debugFlags, 1 << PxVisualizationParameter::eBODY_LIN_VELOCITY );
        stateUpdated |= ImGui::CheckboxFlags( "Angular Velocity", &debugFlags, 1 << PxVisualizationParameter::eBODY_ANG_VELOCITY );
        stateUpdated |= ImGui::CheckboxFlags( "Mass Axes", &debugFlags, 1 << PxVisualizationParameter::eBODY_MASS_AXES );
        stateUpdated |= ImGui::CheckboxFlags( "Joint Limits", &debugFlags, 1 << PxVisualizationParameter::eJOINT_LIMITS );
        stateUpdated |= ImGui::CheckboxFlags( "Joint Local Frames", &debugFlags, 1 << PxVisualizationParameter::eJOINT_LOCAL_FRAMES );

        if ( stateUpdated )
        {
            m_pPhysicsWorldSystem->SetDebugFlags( debugFlags );
            m_pPhysicsWorldSystem->SetDebugDrawDistance( drawDistance );
        }

        ImGui::Separator();

        ImGui::Checkbox( "Draw Dynamic Actor Bounds", &m_pPhysicsWorldSystem->m_drawDynamicActorBounds );
        ImGui::Checkbox( "Draw Kinematic Actor Bounds", &m_pPhysicsWorldSystem->m_drawKinematicActorBounds );

        //-------------------------------------------------------------------------
        // Component Debug
        //-------------------------------------------------------------------------

        ImGui::Separator();

        if ( ImGui::Button( "Show Physics Component List", ImVec2( -1, 0 ) ) )
        {
            m_isComponentWindowOpen = true;
        }

        if ( ImGui::Button( "Show Material Database", ImVec2( -1, 0 ) ) )
        {
            m_isMaterialDatabaseWindowOpen = true;
        }
    }

    void PhysicsDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        if ( m_isComponentWindowOpen )
        {
            if ( pWindowClass != nullptr ) ImGui::SetNextWindowClass( pWindowClass );
            DrawComponentsWindow( context );
        }

        if ( m_isMaterialDatabaseWindowOpen )
        {
            if ( pWindowClass != nullptr ) ImGui::SetNextWindowClass( pWindowClass );
            DrawMaterialDatabaseWindow( context );
        }
    }

    //-------------------------------------------------------------------------

    void PhysicsDebugView::DrawPVDMenu( EntityWorldUpdateContext const& context )
    {
        if ( ImGui::BeginMenu( "PhysX Visual Debugger" ) )
        {
            if ( !m_pPhysicsSystem->IsConnectedToPVD() )
            {
                if ( ImGui::Button( "Connect to PVD", ImVec2( -1, 0 ) ) )
                {
                    m_pPhysicsSystem->ConnectToPVD();
                }
            }
            else
            {
                if ( ImGui::Button( "Disconnect From PVD", ImVec2( -1, 0 ) ) )
                {
                    m_pPhysicsSystem->DisconnectFromPVD();
                }
            }

            ImGui::SliderFloat( "Recording Time (s)", &m_recordingTimeSeconds, 0.25f, 10.0f );
            if ( ImGui::Button( "PVD Timed Recording", ImVec2( -1, 0 ) ) )
            {
                m_pPhysicsSystem->ConnectToPVD( m_recordingTimeSeconds );
            }

            ImGui::EndMenu();
        }
    }

    void PhysicsDebugView::DrawComponentsWindow( EntityWorldUpdateContext const& context )
    {
        TInlineString<50> componentID;
        bool selectedComponentValid = false;

        ImGui::SetNextWindowBgAlpha( 0.5f );
        if ( ImGui::Begin( "Physics Components", &m_isComponentWindowOpen ) )
        {
            for ( PhysicsShapeComponent* pPhysicsComponent : m_pPhysicsWorldSystem->m_physicsShapeComponents )
            {
                bool isSelectedComponent = pPhysicsComponent == m_pSelectedComponent;
                selectedComponentValid |= isSelectedComponent;

                componentID.sprintf( "%s##%u", pPhysicsComponent->GetName().c_str(), pPhysicsComponent->GetID().ToUint64() );
                if ( ImGui::Selectable( componentID.c_str(), isSelectedComponent ) )
                {
                    if ( m_pSelectedComponent != nullptr && isSelectedComponent )
                    {
                        m_pSelectedComponent = nullptr;
                    }
                    else
                    {
                        m_pSelectedComponent = pPhysicsComponent;
                        selectedComponentValid = true;
                    }
                }

                if ( ImGui::IsItemHovered() )
                {
                    auto drawingContext = context.GetDrawingContext();
                    drawingContext.DrawBox( pPhysicsComponent->GetWorldBounds(), Colors::Cyan.GetAlphaVersion( 0.25f ) );
                    drawingContext.DrawWireBox( pPhysicsComponent->GetWorldBounds(), Colors::Cyan );
                }
            }
        }
        ImGui::End();

        //-------------------------------------------------------------------------

        if ( selectedComponentValid )
        {
            if ( m_pSelectedComponent != nullptr )
            {
                DrawComponentVisualization( context, reinterpret_cast<PhysicsShapeComponent*>( m_pSelectedComponent ) );
            }
        }
        else
        {
            m_pSelectedComponent = nullptr;
        }
    }

    void PhysicsDebugView::DrawComponentVisualization( EntityWorldUpdateContext const& context, PhysicsShapeComponent const* pComponent ) const
    {
        auto drawingContext = context.GetDrawingContext();

        drawingContext.DrawAxis( pComponent->GetWorldTransform(), 0.5f, 3.0f );

        if ( auto pCapsuleComponent = TryCast<CapsuleComponent>( pComponent ) )
        {
            drawingContext.DrawCapsuleHeightX( pComponent->GetWorldTransform(), pCapsuleComponent->GetRadius(), pCapsuleComponent->GetCapsuleHalfHeight(), Colors::Cyan );
        }
        else if ( auto pSphereComponent = TryCast<SphereComponent>( pComponent ) )
        {
            drawingContext.DrawSphere( pComponent->GetWorldTransform(), Float3( pSphereComponent->GetRadius() ), Colors::Cyan );
        }
        else if ( auto pBoxComponent = TryCast<BoxComponent>( pComponent ) )
        {
            drawingContext.DrawBox( pComponent->GetWorldTransform(), pBoxComponent->GetExtents(), Colors::Cyan );
        }
    }

    void PhysicsDebugView::DrawMaterialDatabaseWindow( EntityWorldUpdateContext const& context )
    {
        if ( m_isMaterialDatabaseWindowOpen )
        {
            m_isMaterialDatabaseWindowOpen = DrawMaterialDatabaseView( context );
        }
    }
}
#endif