#include "EntityEditor_Utils.h"
#include "EntityEditor_Context.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/FileSystem/FileRegistry.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "Engine/Entity/Entity.h"
#include "Base/Resource/ResourceTypeID.h"
#include "Engine/Imgui/ImguiGizmo.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    bool ViewportResourceDropHandler::CanCreateEntitiesFromResourceType(ResourceTypeID resourceTypeID ) const
    {
        if ( resourceTypeID == Render::StaticMesh::GetStaticResourceTypeID() )
        {
            return true;
        }

        return false;
    }

    void ViewportResourceDropHandler::CreateEntitiesFromResource( EditorContext& context, EntityMap* pMap, ResourceID const& resourceID, Vector const& worldPosition ) const
    {
        EE_ASSERT( pMap != nullptr );
        EE_ASSERT( resourceID.IsValid() );
        EE_ASSERT( worldPosition.IsValid() );

        //-------------------------------------------------------------------------

        // Create entity
        StringID const entityName( resourceID.GetFilenameWithoutExtension().c_str() );
        auto pMeshEntity = New<Entity>( entityName );

        // Create mesh component
        auto pMeshComponent = New<Render::StaticMeshComponent>( StringID( "Mesh Component" ) );
        pMeshComponent->SetMesh( resourceID );
        pMeshComponent->SetWorldTransform( Transform( Quaternion::Identity, worldPosition ) );
        pMeshEntity->AddComponent( pMeshComponent );

        // Try create optional physics collision component
        DataPath physicsResourcePath = resourceID.GetDataPath();
        physicsResourcePath.ReplaceExtension( Physics::CollisionMesh::GetStaticResourceTypeID().ToString() );
        ResourceID const physicsResourceID( physicsResourcePath );
        if ( context.m_pToolsContext->m_pFileRegistry->DoesFileExist( physicsResourceID ) )
        {
            auto pPhysicsMeshComponent = New<Physics::CollisionMeshComponent>( StringID( "Physics Component" ) );
            pPhysicsMeshComponent->SetCollisionMesh( physicsResourceID );
            pMeshEntity->AddComponent( pPhysicsMeshComponent );
        }

        // Add entity to map
        context.AddEntityToMap( pMap, pMeshEntity );
    }

    //-------------------------------------------------------------------------

    struct MenuItemSizes
    {
        MenuItemSizes( ImGuiX::Gizmo &gizmo )
        {
            auto const& style = ImGui::GetStyle();
            m_widthButton0 = ImGui::CalcTextSize( gizmo.IsInWorldSpace() ? EE_ICON_EARTH : EE_ICON_MAP_MARKER ).x + ( style.FramePadding.x * 2 );
            m_widthButton1 = ImGui::CalcTextSize( EE_ICON_AXIS_ARROW ).x + ( style.FramePadding.x * 2 );
            m_widthButton2 = ImGui::CalcTextSize( EE_ICON_ROTATE_ORBIT ).x + ( style.FramePadding.x * 2 );
            m_widthButton3 = ImGui::CalcTextSize( EE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT ).x + ( style.FramePadding.x * 2 );
            m_requiredWidth = m_widthButton0 + m_widthButton1 + m_widthButton2 + m_widthButton3 + m_separatorWidth;
        }

    public:

        float m_separatorWidth = 3;
        float m_widthButton0 = 0;
        float m_widthButton1 = 0;
        float m_widthButton2 = 0;
        float m_widthButton3 = 0;
        float m_requiredWidth = 0;
    };

    float GizmoViewportControls::GetRequiredSize( ImGuiX::Gizmo &gizmo )
    {
        MenuItemSizes ms( gizmo );
        return ms.m_requiredWidth;
    }

    void GizmoViewportControls::Draw( ImGuiX::Gizmo &gizmo )
    {
        MenuItemSizes ms( gizmo );

        //-------------------------------------------------------------------------

        TInlineString<10> const coordinateSpaceSwitcherLabel( TInlineString<10>::CtorSprintf(), "%s##CoordinateSpace", gizmo.IsInWorldSpace() ? EE_ICON_EARTH : EE_ICON_MAP_MARKER );
        if ( ImGuiX::FlatButton( coordinateSpaceSwitcherLabel.c_str(), ImVec2( ms.m_widthButton0, ImGui::GetContentRegionAvail().y ) ) )
        {
            gizmo.SetCoordinateSystemSpace( gizmo.IsInWorldSpace() ? CoordinateSpace::Local : CoordinateSpace::World );
        }
        ImGuiX::ItemTooltip( "Current Mode: %s", gizmo.IsInWorldSpace() ? "World Space" : "Local Space" );

        //-------------------------------------------------------------------------

        ImGuiX::SameLineSeparator( ms.m_separatorWidth );

        //-------------------------------------------------------------------------

        bool t = gizmo.GetMode() == ImGuiX::Gizmo::Mode::Translation;
        ImGui::PushStyleColor( ImGuiCol_Text, t ? ImGuiX::Style::s_colorAccent1 : ImGuiX::Style::s_colorText );
        if ( ImGuiX::FlatButton( EE_ICON_AXIS_ARROW, ImVec2( ms.m_widthButton1, ImGui::GetContentRegionAvail().y ) ) )
        {
            gizmo.SetMode( ImGuiX::Gizmo::Mode::Translation );
        }
        ImGui::PopStyleColor();
        ImGuiX::ItemTooltip( "Translate" );

        ImGui::SameLine( 0, 0 );

        bool r = gizmo.GetMode() == ImGuiX::Gizmo::Mode::Rotation;
        ImGui::PushStyleColor( ImGuiCol_Text, r ? ImGuiX::Style::s_colorAccent1 : ImGuiX::Style::s_colorText );
        if ( ImGuiX::FlatButton( EE_ICON_ROTATE_ORBIT, ImVec2( ms.m_widthButton2, ImGui::GetContentRegionAvail().y ) ) )
        {
            gizmo.SetMode( ImGuiX::Gizmo::Mode::Rotation );
        }
        ImGui::PopStyleColor();
        ImGuiX::ItemTooltip( "Rotate" );

        ImGui::SameLine( 0, 0 );

        bool s = gizmo.GetMode() == ImGuiX::Gizmo::Mode::Scale;
        ImGui::PushStyleColor( ImGuiCol_Text, s ? ImGuiX::Style::s_colorAccent1 : ImGuiX::Style::s_colorText );
        if ( ImGuiX::FlatButton( EE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT, ImVec2( ms.m_widthButton3, ImGui::GetContentRegionAvail().y ) ) )
        {
            gizmo.SetMode( ImGuiX::Gizmo::Mode::Scale );
        }
        ImGui::PopStyleColor();
        ImGuiX::ItemTooltip( "Scale" );
    }

}