#include "Workspace_BoneMask.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationBoneMask.h"
#include "EngineTools/Core/Widgets/InterfaceHelpers.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "System/Math/MathStringHelpers.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_WORKSPACE_FACTORY( BoneMaskWorkspaceFactory, BoneMaskDefinition, BoneMaskWorkspace );

    //-------------------------------------------------------------------------

    static bool IsValidWeight( float weight )
    {
        return ( weight >= 0.0f && weight <= 1.0f ) || weight == -1.0f;
    }

    static Color GetColorForWeight( float w )
    {
        EE_ASSERT( IsValidWeight( w ) );

        // 0%
        if ( w <= 0.0f )
        {
            return Colors::Gray;
        }
        // 1~20%
        else if ( w > 0.0f && w < 0.2f )
        {
            return Color( 0xFF0D0DFF );
        }
        // 20~40%
        else if ( w >= 0.2f && w < 0.4f )
        {
            return Color( 0xFF4E11FF );
        }
        // 40~60%
        else if ( w >= 0.4f && w < 0.6f )
        {
            return Color( 0xFF8E15FF );
        }
        // 60~80%
        else if ( w >= 0.6f && w < 0.8f )
        {
            return Color( 0xFAB733FF );
        }
        // 80~99%
        else if ( w >= 0.08f && w < 1.0f )
        {
            return Color( 0xACB334FF );
        }
        // 100%
        else if ( w == 1.0f )
        {
            return Color( 0x69B34CFF );
        }

        return Colors::White;
    }

    //-------------------------------------------------------------------------

    BoneMaskWorkspace::~BoneMaskWorkspace()
    {
        EE_ASSERT( !m_skeleton.IsSet() );
        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
    }

    void BoneMaskWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID viewportDockID = 0;
        ImGuiID leftDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.3f, nullptr, &viewportDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetViewportWindowID(), viewportDockID );
        ImGui::DockBuilderDockWindow( m_weightEditorWindowName.c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( m_descriptorWindowName.c_str(), leftDockID );
    }

    //-------------------------------------------------------------------------

    void BoneMaskWorkspace::Initialize( UpdateContext const& context )
    {
        TWorkspace<BoneMaskDefinition>::Initialize( context );
        m_weightEditorWindowName.sprintf( "Weights##%u", GetID() );

        //-------------------------------------------------------------------------

        LoadSkeleton();
    }

    void BoneMaskWorkspace::Shutdown( UpdateContext const& context )
    {
        UnloadSkeleton();

        TWorkspace<BoneMaskDefinition>::Shutdown( context );
    }

    void BoneMaskWorkspace::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        TWorkspace<BoneMaskDefinition>::PostUndoRedo( operation, pAction );
        
        // Reflect the actual bone weights into the working set
        if ( m_skeleton.IsLoaded() )
        {
            auto pBoneMaskDesc = GetDescriptor<BoneMaskResourceDescriptor>();
            for ( auto i = 0; i < m_skeleton->GetNumBones(); i++ )
            {
                pBoneMaskDesc->m_boneWeights[i] = Math::Clamp( pBoneMaskDesc->m_boneWeights[i], 0.0f, 1.0f );
                m_workingSetWeights[i] = pBoneMaskDesc->m_boneWeights[i];
                UpdateDemoBoneMask();
            }
        }
    }

    void BoneMaskWorkspace::OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded )
    {
        TWorkspace<BoneMaskDefinition>::OnHotReloadStarted( descriptorNeedsReload, resourcesToBeReloaded );

        // Destroy the tree since the skeleton will need to be reloaded
        DestroySkeletonTree();

        // If the descriptor is being reloaded, unload the skeleton
        if ( descriptorNeedsReload )
        {
            UnloadSkeleton();
        }
    }

    void BoneMaskWorkspace::OnHotReloadComplete()
    {
        // Try to load the skeleton (handles invalid descriptor now becoming valid)
        if ( !m_skeleton.IsSet() )
        {
            LoadSkeleton();
        }

        TWorkspace<BoneMaskDefinition>::OnHotReloadComplete();
    }

    //-------------------------------------------------------------------------

    void BoneMaskWorkspace::LoadSkeleton()
    {
        auto pBoneMaskDesc = GetDescriptor<BoneMaskResourceDescriptor>();
        if ( pBoneMaskDesc->m_skeleton.IsSet() )
        {
            m_skeleton = pBoneMaskDesc->m_skeleton;
            LoadResource( &m_skeleton );
        }
    }

    void BoneMaskWorkspace::UnloadSkeleton()
    {
        DestroySkeletonTree();

        if ( m_skeleton.IsSet() && m_skeleton.WasRequested() )
        {
            UnloadResource( &m_skeleton );
        }
        m_skeleton.Clear();
    }

    void BoneMaskWorkspace::CreateSkeletonTree()
    {
        TVector<BoneInfo*> boneInfos;

        // Create all infos
        int32_t const numBones = m_skeleton->GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& pBoneInfo = boneInfos.emplace_back( EE::New<BoneInfo>() );
            pBoneInfo->m_boneIdx = i;
        }

        // Create hierarchy
        for ( auto i = 1; i < numBones; i++ )
        {
            int32_t const parentBoneIdx = m_skeleton->GetParentBoneIndex( i );
            EE_ASSERT( parentBoneIdx != InvalidIndex );
            boneInfos[parentBoneIdx]->m_children.emplace_back( boneInfos[i] );
        }

        // Set root
        m_pSkeletonTreeRoot = boneInfos[0];
    }

    void BoneMaskWorkspace::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    void BoneMaskWorkspace::CreateDescriptorWeights()
    {
        auto pBoneMaskDesc = GetDescriptor<BoneMaskResourceDescriptor>();

        // Ensure valid number of weights
        if ( pBoneMaskDesc->m_boneWeights.size() != m_skeleton->GetNumBones() )
        {
            pBoneMaskDesc->m_boneWeights.resize( m_skeleton->GetNumBones(), 1.0f );
        }

        m_workingSetWeights.resize( m_skeleton->GetNumBones() );

        // Root weight is always 0
        pBoneMaskDesc->m_boneWeights[0] = 0.0f;

        // Ensure valid weight values
        for ( auto i = 0; i < m_skeleton->GetNumBones(); i++ )
        {
            if ( pBoneMaskDesc->m_boneWeights[i] != -1.0f )
            {
                pBoneMaskDesc->m_boneWeights[i] = Math::Clamp( pBoneMaskDesc->m_boneWeights[i], 0.0f, 1.0f );
            }
            m_workingSetWeights[i] = pBoneMaskDesc->m_boneWeights[i];
        }

        UpdateDemoBoneMask();
    }

    void BoneMaskWorkspace::SetWeight( int32_t boneIdx, float weight )
    {
        EE_ASSERT( IsValidWeight( weight ) );
        EE_ASSERT( m_skeleton.IsSet() && m_skeleton.IsLoaded() );
        EE_ASSERT( m_skeleton->IsValidBoneIndex( boneIdx ) );

        ScopedDescriptorModification const sm( this );
        auto pBoneMaskDesc = GetDescriptor<BoneMaskResourceDescriptor>();
        pBoneMaskDesc->m_boneWeights[boneIdx] = weight;
        m_workingSetWeights[boneIdx] = weight;

        UpdateDemoBoneMask();
    }

    void BoneMaskWorkspace::SetAllChildWeights( int32_t parentBoneIdx, float weight, bool bIncludeParent )
    {
        EE_ASSERT( IsValidWeight( weight ) );
        EE_ASSERT( m_skeleton.IsSet() && m_skeleton.IsLoaded() );
        EE_ASSERT( m_skeleton->IsValidBoneIndex( parentBoneIdx ) );

        auto pBoneMaskDesc = GetDescriptor<BoneMaskResourceDescriptor>();

        ScopedDescriptorModification const sm( this );
        
        if ( bIncludeParent )
        {
            pBoneMaskDesc->m_boneWeights[parentBoneIdx] = weight;
        }
        
        auto const numBones = m_skeleton->GetNumBones();
        for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
        {
            if ( m_skeleton->IsChildBoneOf( parentBoneIdx, boneIdx ) )
            {
                pBoneMaskDesc->m_boneWeights[boneIdx] = weight;
                m_workingSetWeights[boneIdx] = weight;
            }
        }

        UpdateDemoBoneMask();
    }

    void BoneMaskWorkspace::UpdateDemoBoneMask()
    {
        EE_ASSERT( m_skeleton.IsSet() && m_skeleton.IsLoaded() );

        BoneMaskDefinition boneMaskDef;

        auto const numSkeletonBones = m_skeleton->GetNumBones();
        for ( auto i = 0; i < numSkeletonBones; i++ )
        {
            if ( m_workingSetWeights[i] == -1.0f || m_workingSetWeights[i] == 0.0f )
            {
                continue;
            }

            StringID const boneID = m_skeleton->GetBoneID( i );
            boneMaskDef.m_weights.emplace_back( boneID, m_workingSetWeights[i] );
        }

        m_demoBoneMask = BoneMask( m_skeleton.GetPtr(), boneMaskDef, 0.0f );
    }

    //-------------------------------------------------------------------------

    void BoneMaskWorkspace::DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        if ( !m_skeleton.IsSet() )
        {
            auto size = ImGui::GetContentRegionAvail();

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Large, Colors::Red );
                static constexpr char const* const msg = "Invalid Descriptor!";
                auto const ts = ImGui::CalcTextSize( msg );
                ImGui::SetCursorPosX( ( size.x - ts.x ) / 2 );
                ImGui::SetCursorPosY( size.y / 2 - ts.y );
                ImGui::Text( msg );
            }

            {
                ImGuiX::ScopedFont const sf( ImGuiX::Font::Large, Colors::Red );
                static constexpr char const* const msg = "Bone masks need to be based on a skeleton! Please set the skeleton resource in the descriptor!";
                auto const ts = ImGui::CalcTextSize( msg );
                ImGui::SetCursorPosX( ( size.x - ts.x ) / 2 );
                ImGui::Text( msg );
            }
        }
    }

    void BoneMaskWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        TWorkspace::Update( context, pWindowClass, isFocused );

        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() && m_skeleton.IsLoaded() )
        {
            // Lazy create the skeleton tree root and bone mask data whenever the skeleton is loaded
            if ( m_pSkeletonTreeRoot == nullptr )
            {
                CreateDescriptorWeights();
                CreateSkeletonTree();
            }
        }

        //-------------------------------------------------------------------------

        DrawMaskPreview();
        DrawWeightEditorWindow( context, pWindowClass );
    }

    void BoneMaskWorkspace::DrawWeightEditorWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_weightEditorWindowName.c_str() ) )
        {
            if ( m_pSkeletonTreeRoot != nullptr )
            {
                static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

                if ( ImGui::BeginTable( "WeightEditor", 2, flags ) )
                {
                    // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
                    ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_NoHide );
                    ImGui::TableSetupColumn( "Weight", ImGuiTableColumnFlags_WidthFixed, 10 * 12.0f );
                    ImGui::TableHeadersRow();

                    DrawBoneWeightEditor( m_pSkeletonTreeRoot );
                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }

    void BoneMaskWorkspace::DrawBoneWeightEditor( BoneInfo* pBone )
    {
        EE_ASSERT( pBone != nullptr );
        auto pBoneMaskDesc = GetDescriptor<BoneMaskResourceDescriptor>();

        //-------------------------------------------------------------------------

        int32_t const boneIdx = pBone->m_boneIdx;
        StringID const currentBoneID = m_skeleton->GetBoneID( boneIdx );
        float* pWeight = &m_workingSetWeights[boneIdx];

        InlineString boneLabel;
        boneLabel.sprintf( "%d. %s", boneIdx, currentBoneID.c_str() );

        InlineString weightEditorID;
        weightEditorID.sprintf( "##%s", currentBoneID.c_str() );

        InlineString checkboxID;
        checkboxID.sprintf( "##%s_cb", currentBoneID.c_str() );

        InlineString contextMenuID;
        contextMenuID.sprintf( "##%s_ctx", currentBoneID.c_str() );

        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( true );
        int32_t treeNodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth;

        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        // Draw Row
        //-------------------------------------------------------------------------

        ImGui::PushStyleColor( ImGuiCol_Text, GetColorForWeight( *pWeight ).ToUInt32_ABGR() );
        ImGui::TableNextRow();

        //-------------------------------------------------------------------------
        // Draw bone ID and context menu
        //-------------------------------------------------------------------------

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );

        ImGui::PushStyleColor( ImGuiCol_Text, ImU32( ImGuiX::Style::s_colorText ) );
        if ( ImGui::BeginPopupContextItem( contextMenuID.c_str() ) )
        {
            if ( boneIdx != 0 && ImGui::MenuItem( "Set bone and all children to 0" ) )
            {
                ScopedDescriptorModification const sm( this );
                SetAllChildWeights( boneIdx, 0.0f, true );
            }

            if ( ImGui::MenuItem( "Set all children to 0" ) )
            {
                SetAllChildWeights( boneIdx, 0.0f );
            }

            //-------------------------------------------------------------------------

            ImGui::Separator();

            if ( boneIdx != 0 && ImGui::MenuItem( "Set bone and all children to 1" ) )
            {
                ScopedDescriptorModification const sm( this );
                SetAllChildWeights( boneIdx, 1.0f, true );
            }

            if ( ImGui::MenuItem( "Set all children to 1" ) )
            {
                SetAllChildWeights( boneIdx, 1.0f );
            }

            //-------------------------------------------------------------------------

            ImGui::Separator();

            if ( boneIdx != 0 && ImGui::MenuItem( "Unset bone and all children" ) )
            {
                ScopedDescriptorModification const sm( this );
                SetAllChildWeights( boneIdx, -1.0f, true );
            }

            if ( ImGui::MenuItem( "Unset all children" ) )
            {
                SetAllChildWeights( boneIdx, -1.0f );
            }

            //-------------------------------------------------------------------------

            ImGui::Separator();

            if ( ImGui::MenuItem( "Copy bone weight to all children" ) )
            {
                SetAllChildWeights( boneIdx, *pWeight );
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------
        // Draw Weight Editor
        //-------------------------------------------------------------------------

        ImGui::TableNextColumn();

        if ( boneIdx != 0 )
        {
            bool isSet = m_workingSetWeights[boneIdx] != -1.0f;
            if ( ImGui::Checkbox( checkboxID.c_str(), &isSet ) )
            {
                SetWeight( boneIdx, isSet ? 1.0f : -1.0f );
            }

            if ( isSet )
            {
                ImGui::SameLine();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( weightEditorID.c_str(), pWeight, 0.0f, 1.0f, "%.2f" );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    SetWeight( boneIdx, *pWeight );
                }
            }
            else
            {
                ImGui::SameLine();
                float const demoWeight = m_demoBoneMask.GetWeight( boneIdx );
                ImGui::PushStyleColor( ImGuiCol_Text, GetColorForWeight( demoWeight ).ToUInt32_ABGR() );
                ImGui::Text( "%.2f", demoWeight );
                ImGui::PopStyleColor();
            }
        }

        //-------------------------------------------------------------------------

        ImGui::PopStyleColor();

        // Draw Children
        //-------------------------------------------------------------------------

        if ( pBone->m_isExpanded )
        {
            for ( BoneInfo* pChild : pBone->m_children )
            {
                DrawBoneWeightEditor( pChild );
            }
            ImGui::TreePop();
        }
    }

    void BoneMaskWorkspace::DrawMaskPreview()
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        if ( !m_skeleton.IsSet() || !m_skeleton.IsLoaded() )
        {
            return;
        }

        if ( !m_demoBoneMask.IsValid() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto pBoneMaskDesc = GetDescriptor<BoneMaskResourceDescriptor>();
        auto drawingCtx = GetDrawingContext();
        auto const& localRefPose = m_skeleton->GetLocalReferencePose();
        auto const& parentIndices = m_skeleton->GetParentBoneIndices();

        auto const numBones = localRefPose.size();
        if ( numBones > 0 )
        {
            TInlineVector<Transform, 256> globalTransforms;
            globalTransforms.resize( numBones );

            globalTransforms[0] = localRefPose[0];
            for ( auto i = 1; i < numBones; i++ )
            {
                auto const& parentIdx = parentIndices[i];
                auto const& parentTransform = globalTransforms[parentIdx];
                globalTransforms[i] = localRefPose[i] * parentTransform;
            }

            //-------------------------------------------------------------------------

            for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
            {
                auto const& parentIdx = parentIndices[boneIdx];
                auto const& parentTransform = globalTransforms[parentIdx];
                auto const& boneTransform = globalTransforms[boneIdx];

                Color const boneColor = GetColorForWeight( m_demoBoneMask.GetWeight( boneIdx ) );
                drawingCtx.DrawLine( boneTransform.GetTranslation().ToFloat3(), parentTransform.GetTranslation().ToFloat3(), boneColor, 5.0f );
            }
        }
    }
}