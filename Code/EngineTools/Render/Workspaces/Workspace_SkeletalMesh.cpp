#include "Workspace_SkeletalMesh.h"
#include "EngineTools/Core/Widgets/InterfaceHelpers.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "System/Math/MathStringHelpers.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_WORKSPACE_FACTORY( SkeletalMeshWorkspaceFactory, SkeletalMesh, SkeletalMeshWorkspace );

    //-------------------------------------------------------------------------

    SkeletalMeshWorkspace::~SkeletalMeshWorkspace()
    {
        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
    }

    void SkeletalMeshWorkspace::Initialize( UpdateContext const& context )
    {
        TWorkspace<SkeletalMesh>::Initialize( context );

        m_showDescriptorEditor = true;

        //-------------------------------------------------------------------------

        m_skeletonTreeWindowName.sprintf( "Skeleton##%u", GetID() );
        m_meshInfoWindowName.sprintf( "Mesh Info##%u", GetID() );
        m_detailsWindowName.sprintf( "Details##%u", GetID() );

        //-------------------------------------------------------------------------

        m_pMeshComponent = EE::New<SkeletalMeshComponent>( StringID( "Skeletal Mesh Component" ) );
        m_pMeshComponent->SetMesh( m_workspaceResource.GetResourceID() );

        // We dont own the entity as soon as we add it to the map
        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        m_pPreviewEntity->AddComponent( m_pMeshComponent );
        AddEntityToWorld( m_pPreviewEntity );
    }

    void SkeletalMeshWorkspace::Shutdown( UpdateContext const& context )
    {
        DestroySkeletonTree();
        TWorkspace<SkeletalMesh>::Shutdown( context );
    }

    void SkeletalMeshWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID viewportDockID = 0;
        ImGuiID leftTopDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.3f, nullptr, &viewportDockID );
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Down, 0.2f, nullptr, &viewportDockID );
        ImGuiID leftBottomDockID = ImGui::DockBuilderSplitNode( leftTopDockID, ImGuiDir_Down, 0.2f, nullptr, &leftTopDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetViewportWindowID(), viewportDockID );
        ImGui::DockBuilderDockWindow( m_descriptorWindowName.c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( m_meshInfoWindowName.c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( m_skeletonTreeWindowName.c_str(), leftTopDockID );
        ImGui::DockBuilderDockWindow( m_detailsWindowName.c_str(), leftBottomDockID );
    }

    void SkeletalMeshWorkspace::DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ImGui::SetNextItemWidth( 46 );
        if ( ImGui::BeginCombo( "##MeshOptions", EE_ICON_COG, ImGuiComboFlags_HeightLarge ) )
        {
            ImGui::MenuItem( "Show Normals", nullptr, &m_showNormals );
            ImGui::MenuItem( "Show Vertices", nullptr, &m_showVertices );
            ImGui::MenuItem( "Show Bind Pose", nullptr, &m_showBindPose );
            ImGui::MenuItem( "Show Bounds", nullptr, &m_showBounds );

            ImGui::EndCombo();
        }
    }

    void SkeletalMeshWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        TWorkspace::Update( context, pWindowClass, isFocused );

        if ( IsResourceLoaded() )
        {
            auto drawingCtx = GetDrawingContext();

            if ( m_showBounds )
            {
                drawingCtx.DrawWireBox( m_workspaceResource->GetBounds(), Colors::Cyan );
            }

            if ( m_showBindPose )
            {
                m_workspaceResource->DrawBindPose( drawingCtx, Transform::Identity );
            }

            if ( m_showBindPose )
            {
                if ( m_pMeshComponent != nullptr && m_pMeshComponent->IsInitialized() )
                {
                    m_pMeshComponent->DrawPose( drawingCtx );
                }
            }

            if ( m_showVertices || m_showNormals )
            {
                auto pVertex = reinterpret_cast<StaticMeshVertex const*>( m_workspaceResource->GetVertexData().data() );
                for ( auto i = 0; i < m_workspaceResource->GetNumVertices(); i++ )
                {
                    if ( m_showVertices )
                    {
                        drawingCtx.DrawPoint( pVertex->m_position, Colors::Cyan );
                    }

                    if ( m_showNormals )
                    {
                        drawingCtx.DrawLine( pVertex->m_position, pVertex->m_position + ( pVertex->m_normal * 0.15f ), Colors::Yellow );
                    }
                    pVertex++;
                }
            }

            if ( m_selectedBoneID.IsValid() )
            {
                int32_t const selectedBoneIdx = m_workspaceResource->GetBoneIndex( m_selectedBoneID );
                if ( selectedBoneIdx != InvalidIndex )
                {
                    Transform const& globalBoneTransform = m_workspaceResource->GetBindPose()[selectedBoneIdx];
                    drawingCtx.DrawAxis( globalBoneTransform, 0.25f, 3.0f );

                    Vector textLocation = globalBoneTransform.GetTranslation();
                    Vector const textLineLocation = textLocation - Vector( 0, 0, 0.01f );
                    drawingCtx.DrawText3D( textLocation, m_selectedBoneID.c_str(), Colors::Yellow );
                }
            }
        }

        //-------------------------------------------------------------------------

        ImGui::SetNextWindowClass( pWindowClass );
        DrawMeshInfoWindow( context );

        ImGui::SetNextWindowClass( pWindowClass );
        DrawSkeletonTreeWindow( context );

        ImGui::SetNextWindowClass( pWindowClass );
        DrawDetailsWindow( context );
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshWorkspace::DrawMeshInfoWindow( UpdateContext const& context )
    {
        if ( ImGui::Begin( m_meshInfoWindowName.c_str() ) )
        {
            if ( IsWaitingForResource() )
            {
                ImGui::Text( "Loading:" );
                ImGui::SameLine();
                ImGuiX::DrawSpinner( "Loading" );
            }
            else if ( HasLoadingFailed() )
            {
                ImGui::Text( "Loading Failed: %s", m_workspaceResource.GetResourceID().c_str() );
            }
            else // Draw UI
            {
                auto pMesh = m_workspaceResource.GetPtr();
                EE_ASSERT( pMesh != nullptr );

                //-------------------------------------------------------------------------
                // Draw Mesh Data
                //-------------------------------------------------------------------------

                ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 2 ) );
                if ( ImGui::BeginTable( "MeshInfoTable", 2, ImGuiTableFlags_Borders ) )
                {
                    ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthFixed, 100 );
                    ImGui::TableSetupColumn( "Data", ImGuiTableColumnFlags_NoHide );

                    //-------------------------------------------------------------------------

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text( "Data Path" );

                    ImGui::TableNextColumn();
                    ImGui::Text( pMesh->GetResourceID().c_str() );

                    //-------------------------------------------------------------------------

                    auto const pSkeletalMesh = reinterpret_cast<SkeletalMesh const*>( pMesh );

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text( "Num Bones" );

                    ImGui::TableNextColumn();
                    ImGui::Text( "%d", pSkeletalMesh->GetNumBones() );

                    ImGui::TableNextColumn();
                    ImGui::Text( "Num Vertices" );

                    ImGui::TableNextColumn();
                    ImGui::Text( "%d", pMesh->GetNumVertices() );

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text( "Num Indices" );

                    ImGui::TableNextColumn();
                    ImGui::Text( "%d", pMesh->GetNumIndices() );

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text( "Mesh Sections" );

                    ImGui::TableNextColumn();
                    for ( auto const& section : pMesh->GetSections() )
                    {
                        ImGui::Text( section.m_ID.c_str() );
                    }


                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
            }
        }
        ImGui::End();
    }

    void SkeletalMeshWorkspace::DrawSkeletonTreeWindow( UpdateContext const& context )
    {
        if ( ImGui::Begin( m_skeletonTreeWindowName.c_str() ) )
        {
            if ( IsResourceLoaded() )
            {
                if ( m_pSkeletonTreeRoot == nullptr )
                {
                    CreateSkeletonTree();
                }

                EE_ASSERT( m_pSkeletonTreeRoot != nullptr );
                RenderSkeletonTree( m_pSkeletonTreeRoot );
            }
        }
        ImGui::End();
    }

    void SkeletalMeshWorkspace::DrawDetailsWindow( UpdateContext const& context )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8, 8 ) );
        if ( ImGui::Begin( m_detailsWindowName.c_str() ) )
        {
            if ( IsResourceLoaded() )
            {
                if ( m_selectedBoneID.IsValid() )
                {
                    int32_t const selectedBoneIdx = m_workspaceResource->GetBoneIndex( m_selectedBoneID );
                    if ( selectedBoneIdx != InvalidIndex )
                    {
                        {
                            ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                            ImGui::Text( "%d. %s", selectedBoneIdx, m_workspaceResource->GetBoneID( selectedBoneIdx ).c_str() );
                        }

                        int32_t const parentBoneIdx = m_workspaceResource->GetParentBoneIndex( selectedBoneIdx );
                        if ( parentBoneIdx != InvalidIndex )
                        {
                            ImGui::NewLine();
                            ImGui::Text( "Local Transform" );
                            Transform const& localBoneTransform = Transform::Delta( m_workspaceResource->GetBindPose()[parentBoneIdx], m_workspaceResource->GetBindPose()[selectedBoneIdx] );
                            ImGuiX::DisplayTransform( localBoneTransform );
                        }

                        ImGui::NewLine();
                        ImGui::Text( "Global Transform" );
                        Transform const& globalBoneTransform = m_workspaceResource->GetBindPose()[selectedBoneIdx];
                        ImGuiX::DisplayTransform( globalBoneTransform );
                    }
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshWorkspace::CreateSkeletonTree()
    {
        TVector<BoneInfo*> boneInfos;

        // Create all infos
        int32_t const numBones = m_workspaceResource->GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& pBoneInfo = boneInfos.emplace_back( EE::New<BoneInfo>() );
            pBoneInfo->m_boneIdx = i;
        }

        // Create hierarchy
        for ( auto i = 1; i < numBones; i++ )
        {
            int32_t const parentBoneIdx = m_workspaceResource->GetParentBoneIndex( i );
            EE_ASSERT( parentBoneIdx != InvalidIndex );
            boneInfos[parentBoneIdx]->m_children.emplace_back( boneInfos[i] );
        }

        // Set root
        m_pSkeletonTreeRoot = boneInfos[0];
    }

    void SkeletalMeshWorkspace::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    ImRect SkeletalMeshWorkspace::RenderSkeletonTree( BoneInfo* pBone )
    {
        StringID const currentBoneID = m_workspaceResource->GetBoneID( pBone->m_boneIdx );
        ImGui::SetNextItemOpen( pBone->m_isExpanded );

        int32_t treeNodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        if ( currentBoneID == m_selectedBoneID )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        InlineString boneLabel;
        boneLabel.sprintf( "%d. %s", pBone->m_boneIdx, m_workspaceResource->GetBoneID( pBone->m_boneIdx ).c_str() );
        pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );

        // Handle bone selection
        if ( ImGui::IsItemClicked() )
        {
            m_selectedBoneID = m_workspaceResource->GetBoneID( pBone->m_boneIdx );
        }

        //-------------------------------------------------------------------------

        ImRect const nodeRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

        if ( pBone->m_isExpanded )
        {
            ImColor const TreeLineColor = ImGui::GetColorU32( ImGuiCol_TextDisabled );
            float const SmallOffsetX = 2;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
            verticalLineStart.x += SmallOffsetX;
            ImVec2 verticalLineEnd = verticalLineStart;

            for ( BoneInfo* pChild : pBone->m_children )
            {
                const float HorizontalTreeLineSize = 4.0f;
                const ImRect childRect = RenderSkeletonTree( pChild );
                const float midpoint = ( childRect.Min.y + childRect.Max.y ) / 2.0f;
                drawList->AddLine( ImVec2( verticalLineStart.x, midpoint ), ImVec2( verticalLineStart.x + HorizontalTreeLineSize, midpoint ), TreeLineColor );
                verticalLineEnd.y = midpoint;
            }

            drawList->AddLine( verticalLineStart, verticalLineEnd, TreeLineColor );
            ImGui::TreePop();
        }

        return nodeRect;
    }
}