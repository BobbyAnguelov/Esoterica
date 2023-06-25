#include "Workspace_SkeletalMesh.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "System/Math/MathUtils.h"

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

        //-------------------------------------------------------------------------

        m_pMeshComponent = EE::New<SkeletalMeshComponent>( StringID( "Skeletal Mesh Component" ) );
        m_pMeshComponent->SetMesh( m_workspaceResource.GetResourceID() );

        // We dont own the entity as soon as we add it to the map
        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        m_pPreviewEntity->AddComponent( m_pMeshComponent );
        AddEntityToWorld( m_pPreviewEntity );

        //-------------------------------------------------------------------------

        CreateToolWindow( "Skeleton", [this] ( UpdateContext const& context, bool isFocused ) { DrawSkeletonTreeWindow( context, isFocused ); } );
        CreateToolWindow( "Mesh Info", [this] ( UpdateContext const& context, bool isFocused ) { DrawMeshInfoWindow( context, isFocused ); } );
        CreateToolWindow( "Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawDetailsWindow( context, isFocused ); } );
    }

    void SkeletalMeshWorkspace::Shutdown( UpdateContext const& context )
    {
        DestroySkeletonTree();
        TWorkspace<SkeletalMesh>::Shutdown( context );
    }

    void SkeletalMeshWorkspace::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID viewportDockID = 0;
        ImGuiID leftTopDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.3f, nullptr, &viewportDockID );
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Down, 0.2f, nullptr, &viewportDockID );
        ImGuiID leftBottomDockID = ImGui::DockBuilderSplitNode( leftTopDockID, ImGuiDir_Down, 0.2f, nullptr, &leftTopDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Viewport" ).c_str(), viewportDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Descriptor" ).c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Mesh Info" ).c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Skeleton" ).c_str(), leftTopDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Details" ).c_str(), leftBottomDockID );
    }

    void SkeletalMeshWorkspace::DrawMenu( UpdateContext const& context )
    {
        TWorkspace<SkeletalMesh>::DrawMenu( context );

        //-------------------------------------------------------------------------

        ImGui::Separator();

        if ( ImGui::BeginMenu( EE_ICON_TUNE_VERTICAL"Options" ) )
        {
            ImGui::MenuItem( "Show Normals", nullptr, &m_showNormals );
            ImGui::MenuItem( "Show Vertices", nullptr, &m_showVertices );
            ImGui::MenuItem( "Show Bind Pose", nullptr, &m_showBindPose );
            ImGui::MenuItem( "Show Bounds", nullptr, &m_showBounds );

            ImGui::EndMenu();
        }

        if ( !IsResourceLoaded() )
        {
            return;
        }
    }

    void SkeletalMeshWorkspace::Update( UpdateContext const& context, bool isFocused )
    {
        TWorkspace::Update( context, isFocused );

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
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshWorkspace::DrawMeshInfoWindow( UpdateContext const& context, bool isFocused )
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

    void SkeletalMeshWorkspace::DrawSkeletonTreeWindow( UpdateContext const& context, bool isFocused )
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

    void SkeletalMeshWorkspace::DrawDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        auto DrawTransform = [] ( Transform transform )
        {
            Vector const& translation = transform.GetTranslation();
            Quaternion const& rotation = transform.GetRotation();
            EulerAngles const angles = rotation.ToEulerAngles();
            Float4 const quatValues = rotation.ToFloat4();

            ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
            ImGui::Text( "Rot (Quat): X: %.3f, Y: %.3f, Z: %.3f, W: %.3f", quatValues.m_x, quatValues.m_y, quatValues.m_z, quatValues.m_w );
            ImGui::Text( "Rot (Euler): X: %.3f, Y: %.3f, Z: %.3f", angles.m_x.ToDegrees().ToFloat(), angles.m_y.ToDegrees().ToFloat(), angles.m_z.ToDegrees().ToFloat() );
            ImGui::Text( "Trans: X: %.3f, Y: %.3f, Z: %.3f", translation.GetX(), translation.GetY(), translation.GetZ() );
            ImGui::Text( "Scl: %.3f", transform.GetScale() );
        };

        //-------------------------------------------------------------------------

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
                        DrawTransform( localBoneTransform );
                    }

                    ImGui::NewLine();
                    ImGui::Text( "Global Transform" );
                    Transform const& globalBoneTransform = m_workspaceResource->GetBindPose()[selectedBoneIdx];
                    DrawTransform( globalBoneTransform );
                }
            }
        }
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

        ImColor rowColor = ImGuiX::Style::s_colorText;
        if ( currentBoneID == m_selectedBoneID )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
            rowColor = ImGuiX::Style::s_colorAccent0;
        }

        InlineString boneLabel;
        boneLabel.sprintf( "%d. %s", pBone->m_boneIdx, m_workspaceResource->GetBoneID( pBone->m_boneIdx ).c_str() );
        ImGui::PushStyleColor( ImGuiCol_Text, rowColor.Value );
        pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
        ImGui::PopStyleColor();

        // Handle bone selection
        if ( ImGui::IsItemClicked() )
        {
            m_selectedBoneID = m_workspaceResource->GetBoneID( pBone->m_boneIdx );
        }

        //-------------------------------------------------------------------------

        InlineString const contextMenuID( InlineString::CtorSprintf(), "##%s_ctx", currentBoneID.c_str() );
        if ( ImGui::BeginPopupContextItem( contextMenuID.c_str() ) )
        {
            if ( ImGui::MenuItem( EE_ICON_IDENTIFIER" Copy Bone ID" ) )
            {
                ImGui::SetClipboardText( currentBoneID.c_str() );
            }

            ImGui::EndPopup();
        }

        //-------------------------------------------------------------------------

        ImRect const nodeRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

        if ( pBone->m_isExpanded )
        {
            ImColor const treeLineColor = ImGui::GetColorU32( ImGuiCol_TextDisabled );
            float const smallOffsetX = 2;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
            verticalLineStart.x += smallOffsetX;
            ImVec2 verticalLineEnd = verticalLineStart;

            for ( BoneInfo* pChild : pBone->m_children )
            {
                const float HorizontalTreeLineSize = 4.0f;
                const ImRect childRect = RenderSkeletonTree( pChild );
                const float midpoint = ( childRect.Min.y + childRect.Max.y ) / 2.0f;
                drawList->AddLine( ImVec2( verticalLineStart.x, midpoint ), ImVec2( verticalLineStart.x + HorizontalTreeLineSize, midpoint ), treeLineColor );
                verticalLineEnd.y = midpoint;
            }

            drawList->AddLine( verticalLineStart, verticalLineEnd, treeLineColor );
            ImGui::TreePop();
        }

        return nodeRect;
    }
}