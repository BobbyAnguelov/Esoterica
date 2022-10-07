#include "Workspace_Skeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Core/Widgets/InterfaceHelpers.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "System/Math/MathStringHelpers.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_WORKSPACE_FACTORY( SkeletonWorkspaceFactory, Skeleton, SkeletonWorkspace );

    //-------------------------------------------------------------------------

    SkeletonWorkspace::~SkeletonWorkspace()
    {
        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
    }

    void SkeletonWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID viewportDockID = 0;
        ImGuiID leftDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.3f, nullptr, &viewportDockID );
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Down, 0.2f, nullptr, &viewportDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetViewportWindowID(), viewportDockID );
        ImGui::DockBuilderDockWindow( m_skeletonTreeWindowName.c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( m_descriptorWindowName.c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( m_detailsWindowName.c_str(), bottomDockID );
    }

    //-------------------------------------------------------------------------

    void SkeletonWorkspace::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TWorkspace<Skeleton>::Initialize( context );

        m_skeletonTreeWindowName.sprintf( "Skeleton##%u", GetID() );
        m_detailsWindowName.sprintf( "Details##%u", GetID() );

        CreatePreviewEntity();
    }

    void SkeletonWorkspace::Shutdown( UpdateContext const& context )
    {
        DestroySkeletonTree();
        TWorkspace<Skeleton>::Shutdown( context );
    }

    void SkeletonWorkspace::CreatePreviewEntity()
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        auto pSkeletonDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        if ( pSkeletonDescriptor->m_previewMesh.IsSet() )
        {
            auto pMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
            pMeshComponent->SetSkeleton( m_descriptorID );
            pMeshComponent->SetMesh( pSkeletonDescriptor->m_previewMesh.GetResourceID() );

            m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
            m_pPreviewEntity->AddComponent( pMeshComponent );
            AddEntityToWorld( m_pPreviewEntity );
        }
    }

    void SkeletonWorkspace::OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded )
    {
        TWorkspace<Skeleton>::OnHotReloadStarted( descriptorNeedsReload, resourcesToBeReloaded );
        DestroyEntityInWorld( m_pPreviewEntity );
    }

    void SkeletonWorkspace::OnHotReloadComplete()
    {
        TWorkspace<Skeleton>::OnHotReloadComplete();
        CreatePreviewEntity();
    }

    //-------------------------------------------------------------------------

    void SkeletonWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        TWorkspace::Update( context, pWindowClass, isFocused );

        // Debug drawing in Viewport
        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            auto drawingCtx = GetDrawingContext();

            // Draw skeleton
            drawingCtx.Draw( *m_workspaceResource.GetPtr(), Transform::Identity );

            // Draw selected bone
            if ( m_selectedBoneID.IsValid() )
            {
                int32_t const boneIdx = m_workspaceResource->GetBoneIndex( m_selectedBoneID );
                if ( boneIdx != InvalidIndex )
                {
                    Transform const globalBoneTransform = m_workspaceResource->GetBoneGlobalTransform( boneIdx );
                    drawingCtx.DrawAxis( globalBoneTransform, 0.25f, 3.0f );

                    Vector textLocation = globalBoneTransform.GetTranslation();
                    Vector const textLineLocation = textLocation - Vector( 0, 0, 0.01f );
                    drawingCtx.DrawText3D( textLocation, m_selectedBoneID.c_str(), Colors::Yellow );
                }
            }
        }

        //-------------------------------------------------------------------------

        ImGui::SetNextWindowClass( pWindowClass );
        DrawSkeletonHierarchyWindow( context );

        ImGui::SetNextWindowClass( pWindowClass );
        DrawDetailsWindow( context );
    }

    void SkeletonWorkspace::DrawDetailsWindow( UpdateContext const& context )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8, 8 ) );
        if ( ImGui::Begin( m_detailsWindowName.c_str() ) )
        {
            if ( m_selectedBoneID.IsValid() )
            {
                int32_t const selectedBoneIdx = m_workspaceResource->GetBoneIndex( m_selectedBoneID );
                EE_ASSERT( selectedBoneIdx != InvalidIndex );

                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                    ImGui::Text( "%d. %s", selectedBoneIdx, m_workspaceResource->GetBoneID( selectedBoneIdx ).c_str() );
                }

                ImGui::NewLine();
                ImGui::Text( "Local Transform" );
                Transform const& localBoneTransform = m_workspaceResource->GetLocalReferencePose()[selectedBoneIdx];
                ImGuiX::DisplayTransform( localBoneTransform );

                ImGui::NewLine();
                ImGui::Text( "Global Transform" );
                Transform const& globalBoneTransform = m_workspaceResource->GetGlobalReferencePose()[selectedBoneIdx];
                ImGuiX::DisplayTransform( globalBoneTransform );
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void SkeletonWorkspace::DrawSkeletonHierarchyWindow( UpdateContext const& context )
    {
        if ( ImGui::Begin( m_skeletonTreeWindowName.c_str() ) )
        {
            if ( IsResourceLoaded() )
            {
                if ( m_pSkeletonTreeRoot == nullptr )
                {
                    CreateSkeletonTree();
                }

                ImGui::Text( "Skeleton ID: %s", m_workspaceResource->GetResourceID().c_str() );
                ImGui::Text( "Num Bones: %d", m_workspaceResource->GetNumBones() );

                ImGui::Separator();

                EE_ASSERT( m_pSkeletonTreeRoot != nullptr );
                DrawBone( m_pSkeletonTreeRoot );
            }
        }
        ImGui::End();
    }

    //-------------------------------------------------------------------------

    void SkeletonWorkspace::CreateSkeletonTree()
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

    void SkeletonWorkspace::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    ImRect SkeletonWorkspace::DrawBone( BoneInfo* pBone )
    {
        StringID const currentBoneID = m_workspaceResource->GetBoneID( pBone->m_boneIdx );

        ImGui::SetNextItemOpen( pBone->m_isExpanded );
        int32_t treeNodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        if( currentBoneID == m_selectedBoneID )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        InlineString boneLabel;
        boneLabel.sprintf( "%d. %s", pBone->m_boneIdx, currentBoneID.c_str() );
        pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );

        // Handle bone selection
        if( ImGui::IsItemClicked() )
        {
            m_selectedBoneID = currentBoneID;
        }

        if ( ImGui::BeginPopupContextItem( "BoneCM" ) )
        {
            if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY" Copy Bone ID" ) )
            {
                ImGui::SetClipboardText( currentBoneID.c_str() );
            }
            ImGui::EndPopup();
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
                const ImRect childRect = DrawBone( pChild );
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