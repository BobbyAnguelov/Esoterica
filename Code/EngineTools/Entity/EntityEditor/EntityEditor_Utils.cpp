#include "EntityEditor_Utils.h"
#include "EntityEditor_Context.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/FileSystem/DataFileRegistry.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Components/Component_Decals.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "Engine/Entity/Entity.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Resource/ResourceTypeID.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    bool ViewportResourceDropHandler::CanCreateEntitiesFromResourceType( ResourceTypeID resourceTypeID ) const
    {
        if ( resourceTypeID == Render::StaticMesh::GetStaticResourceTypeID() )
        {
            return true;
        }

        return false;
    }

    void ViewportResourceDropHandler::CreateEntitiesFromResource( EditorContext& context, EntityMap* pMap, ResourceID const& resourceID, Vector const& worldPosition, Vector const& surfaceNormal ) const
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
        if ( context.m_pToolsContext->m_pDataFileRegistry->DoesFileExist( physicsResourceID ) )
        {
            auto pPhysicsMeshComponent = New<Physics::CollisionMeshComponent>( StringID( "Physics Component" ) );
            pPhysicsMeshComponent->SetCollisionMesh( physicsResourceID );
            pMeshEntity->AddComponent( pPhysicsMeshComponent );
        }

        // Add entity to map
        context.AddEntityToMap( pMap, pMeshEntity );
    }

    // Decals
    //-------------------------------------------------------------------------

    bool DecalMaterialViewportResourceDropHandler::CanCreateEntitiesFromResourceType( ResourceTypeID resourceTypeID ) const
    {
        if ( resourceTypeID == Render::DecalMaterial::GetStaticResourceTypeID() )
        {
            return true;
        }

        return false;
    }

    void DecalMaterialViewportResourceDropHandler::CreateEntitiesFromResource( EditorContext& context, EntityMap* pMap, ResourceID const& resourceID, Vector const& worldPosition, Vector const& surfaceNormal ) const
    {
        EE_ASSERT( pMap != nullptr );
        EE_ASSERT( resourceID.IsValid() );
        EE_ASSERT( worldPosition.IsValid() );

        static float const      s_defaultDecalSize = 1.0F;
        static float const      s_defaultDecalThickness = 0.25F;

        //-------------------------------------------------------------------------

        StringID const entityName( resourceID.GetFilenameWithoutExtension().c_str() );
        auto pDecalEntity = New<Entity>( entityName );
        auto pDecalComponent = Cast<Render::DecalComponent>( pDecalEntity->CreateComponent( Render::DecalComponent::GetStaticTypeInfo() ) );

        // A decal projects along its local Z axis, so the surface normal becomes its projection axis.
        // The volume is centred on the drop position, which puts the surface through the middle of it.
        Quaternion const projectionRotation = Quaternion::FromRotationBetweenVectors( Vector::UnitZ, surfaceNormal );
        pDecalComponent->SetWorldTransform( Transform( projectionRotation, worldPosition, s_defaultDecalSize ) );
        pDecalComponent->SetNonUniformScale( Float3( 1.0F, 1.0F, s_defaultDecalThickness ) );

        // Assign the dropped material
        TypeSystem::PropertyInfo const* const pDecalMaterialPropertyInfo = Render::DecalComponent::GetStaticTypeInfo()->GetPropertyInfo( StringID( "m_decalMaterial" ) );
        EE_ASSERT( pDecalMaterialPropertyInfo != nullptr && pDecalMaterialPropertyInfo->IsResourcePtrProperty() );
        *reinterpret_cast<Resource::ResourcePtr*>( pDecalMaterialPropertyInfo->GetPropertyAddress( pDecalComponent ) ) = resourceID;

        // Add entity to map
        context.AddEntityToMap( pMap, pDecalEntity );
    }

    //-------------------------------------------------------------------------

    struct MenuItemSizes
    {
        MenuItemSizes( Gizmo &gizmo )
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

    float GizmoViewportControls::GetRequiredSize( Gizmo &gizmo )
    {
        MenuItemSizes ms( gizmo );
        return ms.m_requiredWidth;
    }

    void GizmoViewportControls::Draw( Gizmo &gizmo )
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

        bool t = gizmo.GetMode() == Gizmo::Mode::Translation;
        ImGui::PushStyleColor( ImGuiCol_Text, t ? ImGuiX::Style::s_colorAccent1 : ImGuiX::Style::s_colorText );
        if ( ImGuiX::FlatButton( EE_ICON_AXIS_ARROW, ImVec2( ms.m_widthButton1, ImGui::GetContentRegionAvail().y ) ) )
        {
            gizmo.SetMode( Gizmo::Mode::Translation );
        }
        ImGui::PopStyleColor();
        ImGuiX::ItemTooltip( "Translate" );

        ImGui::SameLine( 0, 0 );

        bool r = gizmo.GetMode() == Gizmo::Mode::Rotation;
        ImGui::PushStyleColor( ImGuiCol_Text, r ? ImGuiX::Style::s_colorAccent1 : ImGuiX::Style::s_colorText );
        if ( ImGuiX::FlatButton( EE_ICON_ROTATE_ORBIT, ImVec2( ms.m_widthButton2, ImGui::GetContentRegionAvail().y ) ) )
        {
            gizmo.SetMode( Gizmo::Mode::Rotation );
        }
        ImGui::PopStyleColor();
        ImGuiX::ItemTooltip( "Rotate" );

        ImGui::SameLine( 0, 0 );

        bool s = gizmo.GetMode() == Gizmo::Mode::Scale;
        ImGui::PushStyleColor( ImGuiCol_Text, s ? ImGuiX::Style::s_colorAccent1 : ImGuiX::Style::s_colorText );
        if ( ImGuiX::FlatButton( EE_ICON_ARROW_TOP_RIGHT_BOTTOM_LEFT, ImVec2( ms.m_widthButton3, ImGui::GetContentRegionAvail().y ) ) )
        {
            gizmo.SetMode( Gizmo::Mode::Scale );
        }
        ImGui::PopStyleColor();
        ImGuiX::ItemTooltip( "Scale" );
    }

}