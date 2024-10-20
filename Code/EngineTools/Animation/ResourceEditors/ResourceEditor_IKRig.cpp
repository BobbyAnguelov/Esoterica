#include "ResourceEditor_IKRig.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Core/Dialogs.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/UpdateContext.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_EDITOR_FACTORY( IKRigEditorFactory, IKRigDefinition, IKRigEditor );

    //-------------------------------------------------------------------------

    class [[nodiscard]] ScopedIKRigSettingsModification : public ScopedDataFileModification
    {
    public:

        ScopedIKRigSettingsModification( IKRigEditor* pEditor )
            : ScopedDataFileModification( pEditor )
            , m_pIKRigEditor( pEditor )
        {
        }

        virtual ~ScopedIKRigSettingsModification()
        {}

    private:

        IKRigEditor*  m_pIKRigEditor = nullptr;
    };

    //-------------------------------------------------------------------------

    static int32_t GetParentBodyIndex( Animation::Skeleton const* pSkeleton, IKRigDefinition const& definition, int32_t bodyIdx )
    {
        //EE_ASSERT( bodyIdx >= 0 && bodyIdx < definition.m_links.size() );

        //int32_t const boneIdx = pSkeleton->GetBoneIndex( definition.m_links[bodyIdx].m_boneID );

        //// Traverse the hierarchy to try to find the first parent bone that has a body
        //int32_t parentBoneIdx = pSkeleton->GetParentBoneIndex( boneIdx );
        //while ( parentBoneIdx != InvalidIndex )
        //{
        //    // Do we have a body for this parent bone?
        //    StringID const parentBoneID = pSkeleton->GetBoneID( parentBoneIdx );
        //    for ( int32_t i = 0; i < definition.m_links.size(); i++ )
        //    {
        //        if ( definition.m_links[i].m_boneID == parentBoneID )
        //        {
        //            return i;
        //        }
        //    }

        //    parentBoneIdx = pSkeleton->GetParentBoneIndex( parentBoneIdx );
        //}

        return InvalidIndex;
    }

    //-------------------------------------------------------------------------

    IKRigEditor::IKRigEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld )
        : TResourceEditor<IKRigDefinition>( pToolsContext, resourceID, pWorld )
        , m_propertyGrid( pToolsContext )
        , m_resourceFilePicker( *pToolsContext )
    {
        auto OnPreEdit = [this] ( PropertyEditInfo const& info )
        {
            BeginDataFileModification();
        };

        auto OnPostEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );
            EndDataFileModification();
        };

        m_propertyGrid.SetControlBarVisible( false );
        m_preEditEventBindingID = m_propertyGrid.OnPreEdit().Bind( OnPreEdit );
        m_postEditEventBindingID = m_propertyGrid.OnPostEdit().Bind( OnPostEdit );

        //-------------------------------------------------------------------------

        auto CustomResourceFilter = [this] ( Resource::ResourceDescriptor const* pDesc )
        {
            if ( m_skeleton.IsLoaded() )
            {
                AnimationClipResourceDescriptor const* pClipDesc = Cast<AnimationClipResourceDescriptor>( pDesc );
                if ( pClipDesc->m_skeleton == m_skeleton )
                {
                    return true;
                }
            }

            return false;
        };

        m_resourceFilePicker.SetRequiredResourceType( AnimationClip::GetStaticResourceTypeID() );
        m_resourceFilePicker.SetCustomResourceFilter( CustomResourceFilter );

        //-------------------------------------------------------------------------

        m_effectorGizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );
        m_effectorGizmo.SetOption( ImGuiX::Gizmo::Options::AllowCoordinateSpaceSwitching, false );
        m_effectorGizmo.SetMode( ImGuiX::Gizmo::GizmoMode::Translation );
        m_offsetEffectorGizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );
        m_offsetEffectorGizmo.SetOption( ImGuiX::Gizmo::Options::AllowCoordinateSpaceSwitching, false );
        m_offsetEffectorGizmo.SetMode( ImGuiX::Gizmo::GizmoMode::Translation );
    }

    IKRigEditor::~IKRigEditor()
    {
        EE_ASSERT( !IsDebugging() ); 
        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
        EE_ASSERT( m_pPreviewMeshComponent == nullptr );

        m_propertyGrid.OnPreEdit().Unbind( m_preEditEventBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_postEditEventBindingID );
    }

    void IKRigEditor::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID viewportDockID = 0;
        ImGuiID leftDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.3f, nullptr, &viewportDockID );
        ImGuiID bottomleftDockID = ImGui::DockBuilderSplitNode( leftDockID, ImGuiDir_Down, 0.3f, nullptr, &leftDockID );
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Down, 0.2f, nullptr, &viewportDockID );
        ImGuiID rightDockID = ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Right, 0.2f, nullptr, &viewportDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( s_viewportWindowName ).c_str(), viewportDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Rig Editor" ).c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Details" ).c_str(), bottomleftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_dataFileWindowName ).c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Debug Controls" ).c_str(), rightDockID );
    }

    //-------------------------------------------------------------------------

    void IKRigEditor::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TResourceEditor<IKRigDefinition>::Initialize( context );

        CreateToolWindow( "Rig Editor", [this] ( UpdateContext const& context, bool isFocused ) { DrawRigEditorWindow( context, isFocused ); } );
        CreateToolWindow( "Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawRigEditorDetailsWindow( context, isFocused ); } );
        CreateToolWindow( "Debug Controls", [this] ( UpdateContext const& context, bool isFocused ) { DrawDebugControlsWindow( context, isFocused ); } );
    }

    void IKRigEditor::Shutdown( UpdateContext const& context )
    {
        if ( IsDebugging() )
        {
            StopDebugging();
        }

        //-------------------------------------------------------------------------

        if ( m_previewAnimation.IsSet() && m_previewAnimation.WasRequested() )
        {
            UnloadResource( &m_previewAnimation );
        }

        DestroyPreviewEntity();

        TResourceEditor<IKRigDefinition>::Shutdown( context );
    }

    void IKRigEditor::CreatePreviewEntity()
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );
        EE_ASSERT( m_pPreviewMeshComponent == nullptr );
        EE_ASSERT( m_skeleton.IsLoaded() && m_skeleton->IsValid() );

        // Create preview entity
        ResourceID const previewMeshResourceID( m_skeleton->GetPreviewMeshID() );
        if ( previewMeshResourceID.IsValid() )
        {
            m_pPreviewMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
            m_pPreviewMeshComponent->SetSkeleton( m_skeleton->GetResourceID() );
            m_pPreviewMeshComponent->SetMesh( previewMeshResourceID );
            m_pPreviewMeshComponent->SetVisible( m_showPreviewMesh );

            m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
            m_pPreviewEntity->AddComponent( m_pPreviewMeshComponent );
            AddEntityToWorld( m_pPreviewEntity );
        }
    }

    void IKRigEditor::DestroyPreviewEntity()
    {
        if ( m_pPreviewEntity != nullptr )
        {
            DestroyEntityInWorld( m_pPreviewEntity );
            m_pPreviewEntity = nullptr;
            m_pPreviewMeshComponent = nullptr;
        }
    }

    void IKRigEditor::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        TResourceEditor<IKRigDefinition>::PostUndoRedo( operation, pAction );

        if ( IsDebugging() )
        {
            StopDebugging();
        }
    }

    void IKRigEditor::OnDataFileLoadCompleted()
    {
        TResourceEditor<IKRigDefinition>::OnDataFileLoadCompleted();

        if ( IsDataFileLoaded() )
        {
            LoadSkeleton();
        }
    }

    void IKRigEditor::OnDataFileUnload()
    {
        TResourceEditor<IKRigDefinition>::OnDataFileUnload();

        if ( IsDebugging() )
        {
            StopDebugging();
        }

        UnloadSkeleton();
    }

    void IKRigEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource && IsResourceLoaded() )
        {
            // Do Nothing
        }

        if ( pResourcePtr == &m_skeleton && pResourcePtr->IsLoaded() )
        {
            OnSkeletonLoaded();
        }
    }

    void IKRigEditor::OnResourceUnload( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource )
        {
            if ( IsDebugging() )
            {
                StopDebugging();
            }
        }

        //-------------------------------------------------------------------------

        if ( pResourcePtr == &m_skeleton )
        {
            if ( IsDebugging() )
            {
                StopDebugging();
            }

            DestroySkeletonTree();
            DestroyPreviewEntity();
        }
    }

    //-------------------------------------------------------------------------

    void IKRigEditor::LoadSkeleton()
    {
        EE_ASSERT( !m_skeleton.IsSet() || !m_skeleton.WasRequested() );

        m_skeleton = GetRigDescriptor()->m_skeleton;
        if ( m_skeleton.IsSet() )
        {
            LoadResource( &m_skeleton );
        }
        else
        {
            MessageDialog::Error( "Invalid Descriptor", "Descriptor doesnt have a skeleton set!\r\nPlease set a valid skeleton via the descriptor editor" );
        }
    }

    void IKRigEditor::UnloadSkeleton()
    {
        // Destroy all dependent elements
        DestroyPreviewEntity();
        DestroySkeletonTree();

        // Unload skeleton and clear ptr
        if ( m_skeleton.WasRequested() )
        {
            UnloadResource( &m_skeleton );
        }
        m_skeleton.Clear();
    }

    void IKRigEditor::OnSkeletonLoaded()
    {
        CreatePreviewEntity();
        CreateSkeletonTree();
    }

    void IKRigEditor::CreateSkeletonTree()
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

    void IKRigEditor::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    //-------------------------------------------------------------------------

    void IKRigEditor::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        TResourceEditor<IKRigDefinition>::DrawViewportToolbar( context, pViewport );

        if ( !IsResourceLoaded() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        constexpr float const buttonWidth = 180;

        if ( IsDebugging() )
        {
            if ( ImGuiX::IconButton( EE_ICON_STOP, "Stop Debug", Colors::Red, ImVec2( buttonWidth, 0 ) ) )
            {
                StopDebugging();
            }
        }
        else
        {
            ImGui::BeginDisabled( !m_editedResource->IsValid() );
            if ( ImGuiX::IconButton( EE_ICON_PLAY, "Debug IK", Colors::Lime, ImVec2( buttonWidth, 0 ) ) )
            {
                RequestDebugSession();
            }
            ImGui::EndDisabled();
        }
    }

    void IKRigEditor::DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        TResourceEditor<IKRigDefinition>::DrawViewportOverlayElements( context, pViewport );

        if ( !IsDebugging() )
        {
            return;
        }

        // Update effector gizmos
        //-------------------------------------------------------------------------

        auto ApplyManipulation = [] ( ImGuiX::Gizmo::Result const& result, EffectorState& state )
        {
            if ( state.m_behavior == EffectorState::Behavior::Fixed )
            {
                result.ApplyResult( state.m_transform );
            }
            else
            {
                Transform originalWorldTransform = state.m_offsetTransform * state.m_transform;
                Transform modifiedworldTransform = originalWorldTransform;
                result.ApplyResult( modifiedworldTransform );
                state.m_offsetTransform = Transform::Delta( originalWorldTransform, modifiedworldTransform ) * state.m_offsetTransform;
            }
        };

        if ( m_selectedEffectorIndex != InvalidIndex )
        {
            EffectorState& effector = m_effectorDebugStates[m_selectedEffectorIndex];

            Transform const gizmoTransform = effector.m_offsetTransform * effector.m_transform;
            Transform transformDelta = Transform::Identity;

            ImGuiX::Gizmo::Result gizmoResult = m_effectorGizmo.Draw( gizmoTransform.GetTranslation(), gizmoTransform.GetRotation(), *pViewport );
            if ( gizmoResult.m_state == ImGuiX::Gizmo::State::Manipulating )
            {
                ApplyManipulation( gizmoResult, effector );
            }

            //-------------------------------------------------------------------------

            if ( m_showPoseDebug )
            {
                gizmoResult = m_offsetEffectorGizmo.Draw( gizmoTransform.GetTranslation() + m_poseOffset, gizmoTransform.GetRotation(), *pViewport );
                if ( gizmoResult.m_state == ImGuiX::Gizmo::State::Manipulating )
                {
                    ApplyManipulation( gizmoResult, effector );
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    void IKRigEditor::DrawMenu( UpdateContext const& context )
    {
        TResourceEditor<IKRigDefinition>::DrawMenu( context );

        if ( ImGui::BeginMenu( EE_ICON_TUNE" Options" ) )
        {
            ImGui::BeginDisabled( m_pPreviewMeshComponent == nullptr );
            if ( ImGui::Checkbox( "Show Preview Mesh", &m_showPreviewMesh ) )
            {
                m_pPreviewMeshComponent->SetVisible( m_showPreviewMesh );
            }
            ImGui::EndDisabled();

            if ( ImGui::Checkbox( "Show Pose Debug", &m_showPoseDebug ) )
            {
            }

            ImGui::EndMenu();
        }
    }

    void IKRigEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        TResourceEditor::Update( context, isVisible, isFocused );

        //-------------------------------------------------------------------------

        if ( m_debugState == DebugState::RequestStart )
        {
            if ( !IsHotReloading() && !IsLoadingResources() )
            {
                StartDebugging();
            }
        }

        //-------------------------------------------------------------------------

        if ( IsDebugging() )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
            {
                m_effectorGizmo.SwitchToNextMode();
                m_offsetEffectorGizmo.SwitchToNextMode();
            }
        }

        //-------------------------------------------------------------------------

        auto drawingCtx = GetDrawingContext();
        if ( !IsDebugging() && m_skeleton.IsLoaded() )
        {
            IKRigResourceDescriptor* pDescriptor = GetRigDescriptor();
            int32_t const numBodies = (int32_t) pDescriptor->m_links.size();
            for ( int32_t i = 0; i < numBodies; i++ )
            {
                if ( i == 0 )
                {
                    continue;
                }

                int32_t const boneIdx = m_skeleton->GetBoneIndex( pDescriptor->m_links[i].m_boneID );
                int32_t const parentBoneIdx = m_skeleton->GetParentBoneIndex( boneIdx );
                Transform const boneTransform = m_skeleton->GetBoneModelSpaceTransform( boneIdx );
                Transform const parentBoneTransform = m_skeleton->GetBoneModelSpaceTransform( parentBoneIdx );

                // Draw Link
                //-------------------------------------------------------------------------

                drawingCtx.DrawLine( parentBoneTransform.GetTranslation(), boneTransform.GetTranslation(), Colors::Cyan, Colors::Cyan, 5.0f );
                drawingCtx.DrawAxis( boneTransform, Colors::HotPink, Colors::SeaGreen, Colors::RoyalBlue, 0.035f, 4.0f );

                // Draw joint
                //-------------------------------------------------------------------------

                drawingCtx.DrawAxis( parentBoneTransform, 0.025f, 6.0f );
                //ctx.DrawTwistLimits( parentBoneTransform, m_pDefinition->m_links[i].m_minTwistLimit, m_pDefinition->m_links[i].m_maxTwistLimit, Colors::Yellow, 3.0f, 0.01f );
                //ctx.DrawSwingLimits( parentBoneTransform, m_pDefinition->m_links[i].m_swingLimit, m_pDefinition->m_links[i].m_swingLimit, Colors::Orange, 3.0f, 0.02f );
            }
        }
    }

    void IKRigEditor::WorldUpdate( EntityWorldUpdateContext const& updateContext )
    {
        if ( !IsDebugging() )
        {
            return;
        }

        if ( updateContext.GetUpdateStage() != UpdateStage::PrePhysics )
        {
            return;
        }

        auto drawingCtx = GetDrawingContext();
        Seconds const deltaTime = updateContext.GetDeltaTime();

        // Update preview animation
        //-------------------------------------------------------------------------

        Transform const poseOffsetTransform( Quaternion::Identity, m_poseOffset );

        // Update animation time and sample pose
        Percentage const previousAnimTime = m_animTime;
        bool const hasValidAndLoadedPreviewAnim = m_previewAnimation.IsSet() && m_previewAnimation.IsLoaded() && m_previewAnimation->GetSkeleton() == m_skeleton.GetPtr();
        if ( hasValidAndLoadedPreviewAnim )
        {
            if ( m_isPlayingAnimation )
            {
                m_animTime = ( m_animTime + deltaTime / ( m_previewAnimation->GetDuration() ) ).GetClamped( m_enableAnimationLooping );

                // Allow reset
                if ( !m_enableAnimationLooping && m_animTime == 1.0f )
                {
                    m_isPlayingAnimation = false;
                }
            }

            m_previewAnimation->GetPose( m_animTime, m_pPreviewPose );
        }
        else
        {
            m_pPreviewPose->Reset( Animation::Pose::Type::ReferencePose );
        }

        if ( m_showPoseDebug )
        {
            m_pPreviewPose->DrawDebug( drawingCtx, poseOffsetTransform, Skeleton::LOD::High, Colors::Lime, 3.0f );
        }

        // Run Solver
        //-------------------------------------------------------------------------

        m_pFinalPose->CopyFrom( m_pPreviewPose );
        m_pFinalPose->CalculateModelSpaceTransforms();

        // Update effector transforms
        for ( int32_t i = 0; i < m_effectorDebugStates.size(); i++ )
        {
            if ( m_effectorDebugStates[i].m_behavior == EffectorState::Behavior::Follow )
            {
                m_effectorDebugStates[i].m_transform = m_pFinalPose->GetModelSpaceTransform( m_effectorDebugStates[i].m_boneIdx );
            }
        }

        // Set solver target and perform solve
        if ( m_pRig != nullptr )
        {
            for ( int32_t i = 0; i < m_effectorDebugStates.size(); i++ )
            {
                m_pRig->SetEffectorTarget( i, m_effectorDebugStates[i].m_offsetTransform * m_effectorDebugStates[i].m_transform );
            }

            m_pRig->Solve( m_pFinalPose );
        }

        if ( m_showPoseDebug )
        {
            m_pFinalPose->DrawDebug( drawingCtx, poseOffsetTransform, Skeleton::LOD::High, Colors::Orange, 3.0f );
        }

        m_pRig->DrawDebug( drawingCtx, Transform::Identity );

        // Update mesh pose
        //-------------------------------------------------------------------------

        if ( m_pPreviewMeshComponent != nullptr && m_pPreviewMeshComponent->WasInitialized() )
        {
            m_pFinalPose->CalculateModelSpaceTransforms();
            m_pPreviewMeshComponent->SetPose( m_pFinalPose );
            m_pPreviewMeshComponent->FinalizePose();
        }
    }

    //-------------------------------------------------------------------------

    void IKRigEditor::AddBoneToRig( StringID boneID )
    {
        EE_ASSERT( m_skeleton.IsLoaded() );
        EE_ASSERT( GetBodyIndexForBoneID( boneID ) == InvalidIndex );

        IKRigResourceDescriptor* pDescriptor = GetRigDescriptor();

        ScopedIKRigSettingsModification const sdm( this );

        EE_ASSERT( GetBodyIndexForBoneID( boneID ) == InvalidIndex );

        // Bone Info
        //-------------------------------------------------------------------------

        int32_t const boneIdx = m_skeleton->GetBoneIndex( boneID );
        int32_t const parentBoneIdx = m_skeleton->GetParentBoneIndex( boneIdx );
        EE_ASSERT( parentBoneIdx != InvalidIndex );
        Transform const boneTransform = m_skeleton->GetBoneModelSpaceTransform( boneIdx );
        Transform const parentBoneTransform = m_skeleton->GetBoneModelSpaceTransform( parentBoneIdx );

        Vector boneDirection;
        float length = 0.0f;
        ( parentBoneTransform.GetTranslation() - boneTransform.GetTranslation() ).ToDirectionAndLength3( boneDirection, length );
        float const halfLength = length / 2.0f;

        // Create Link
        //-------------------------------------------------------------------------

        IKRigDefinition::Link link;
        link.m_boneID = boneID;
        link.m_mass = 1.0f;
        link.m_radius = Float3( halfLength, 0.025f, 0.025f );

        pDescriptor->m_links.emplace_back( link );
    }

    void IKRigEditor::RemoveBoneFromRig( StringID boneID )
    {
        ScopedIKRigSettingsModification const sdm( this );

        IKRigResourceDescriptor* pDescriptor = GetRigDescriptor();
        EE_ASSERT( m_skeleton.IsLoaded() );

        // Destroy Link
        //-------------------------------------------------------------------------

        int32_t const bodyToDestroyIdx = GetBodyIndexForBoneID( boneID );
        EE_ASSERT( bodyToDestroyIdx != InvalidIndex );
        pDescriptor->m_links.erase( pDescriptor->m_links.begin() + bodyToDestroyIdx );

        // Destroy all children
        //-------------------------------------------------------------------------

        int32_t const boneIdx = m_skeleton->GetBoneIndex( boneID );
        EE_ASSERT( boneIdx != InvalidIndex );

        for ( int32_t bodyIdx = 0; bodyIdx < (int32_t) pDescriptor->m_links.size(); bodyIdx++ )
        {
            int32_t const bodyBoneIdx = m_skeleton->GetBoneIndex( pDescriptor->m_links[bodyIdx].m_boneID );
            if ( m_skeleton->IsChildBoneOf( boneIdx, bodyBoneIdx ) )
            {
                pDescriptor->m_links.erase( pDescriptor->m_links.begin() + bodyIdx );
                bodyIdx--;
            }
        }
    }

    int32_t IKRigEditor::GetBodyIndexForBoneID( StringID boneID ) const
    {
        EE_ASSERT( boneID.IsValid() );

        IKRigResourceDescriptor const* pDescriptor = GetRigDescriptor();
        for ( auto bodyIdx = 0; bodyIdx < pDescriptor->m_links.size(); bodyIdx++ )
        {
            if ( pDescriptor->m_links[bodyIdx].m_boneID == boneID )
            {
                return bodyIdx;
            }
        }

        return InvalidIndex;
    }

    int32_t IKRigEditor::GetBodyIndexForBoneIdx( int32_t boneIdx ) const
    {
        StringID const boneID = m_skeleton->GetBoneID( boneIdx );
        return GetBodyIndexForBoneID( boneID );
    }

    bool IKRigEditor::IsParentBoneInRig( StringID boneID ) const
    {
        EE_ASSERT( m_skeleton.IsLoaded() );
        int32_t const boneIdx = m_skeleton->GetBoneIndex( boneID );
        int32_t const parentBoneIdx = m_skeleton->GetParentBoneIndex( boneIdx );

        if ( parentBoneIdx == InvalidIndex )
        {
            return false;
        }

        StringID const parentBoneID = m_skeleton->GetBoneID( parentBoneIdx );

        IKRigResourceDescriptor const* pDescriptor = GetRigDescriptor();
        for ( auto const& link : pDescriptor->m_links )
        {
            if ( link.m_boneID == parentBoneID )
            {
                return true;
            }
        }

        return false;
    }

    bool IKRigEditor::IsLeafBody( StringID boneID ) const
    {
        int32_t const leafBoneIdx = m_skeleton->GetBoneIndex( boneID );

        IKRigResourceDescriptor const* pDescriptor = GetRigDescriptor();
        for ( auto const& link : pDescriptor->m_links )
        {
            int32_t const boneIdx = m_skeleton->GetBoneIndex( boneID );
            int32_t const parentBoneIdx = m_skeleton->GetParentBoneIndex( boneIdx );
            if ( parentBoneIdx == leafBoneIdx )
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    void IKRigEditor::RenderSkeletonTreeRow( BoneInfo* pBone, int32_t indentLevel )
    {
        EE_ASSERT( pBone != nullptr );

        //-------------------------------------------------------------------------

        StringID const currentBoneID = m_skeleton->GetBoneID( pBone->m_boneIdx );
        int32_t const parentBoneIDx = m_skeleton->GetParentBoneIndex( pBone->m_boneIdx );

        ImGui::TableNextRow();
        ImGui::PushID( pBone );

        // Label
        //-------------------------------------------------------------------------

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();

        bool const requiresIndent = indentLevel > 0;
        float const indentSize = indentLevel * ImGui::GetStyle().IndentSpacing;
        if ( requiresIndent )
        {
            ImGui::Indent( indentSize );
        }

        ImGui::SetNextItemOpen( pBone->m_isExpanded );
        int32_t treeNodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow;

        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        if ( currentBoneID == m_selectedBoneID )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        InlineString boneLabel;
        boneLabel.sprintf( "%d. %s", pBone->m_boneIdx, currentBoneID.c_str() );

        int32_t const bodyIdx = GetBodyIndexForBoneID( currentBoneID );
        bool const hasBody = bodyIdx != InvalidIndex;
        if ( hasBody )
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::MediumBold, Colors::LimeGreen );
            pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
        }
        else
        {
            pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
        }

        // Pop tree item
        if ( pBone->m_isExpanded )
        {
            ImGui::TreePop();
        }

        // Handle bone selection
        if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenBlockedByPopup ) )
        {
            if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
            {
                m_selectedBoneID = currentBoneID;
            }
        }

        // Pop indent level if set
        if ( requiresIndent )
        {
            ImGui::Unindent( indentSize );
        }

        // Controls
        //-------------------------------------------------------------------------

        ImGui::TableNextColumn();

        InlineString str;

        if ( hasBody )
        {
            if ( ImGuiX::ButtonColored( EE_ICON_MINUS, Colors::Red ) )
            {
                RemoveBoneFromRig( currentBoneID );
            }
            str.sprintf( "Remove '%s' from Rig", currentBoneID.c_str() );
            ImGuiX::ItemTooltip( str.c_str() );
        }
        else
        {
            IKRigResourceDescriptor const* pDescriptor = GetRigDescriptor();

            bool const canAddBody = ( pDescriptor->m_links.size() == 0 ) || IsParentBoneInRig( currentBoneID );
            ImGui::BeginDisabled( !canAddBody );
            if ( ImGuiX::ButtonColored( EE_ICON_PLUS, Colors::Lime ) )
            {
                AddBoneToRig( currentBoneID );
            }
            str.sprintf( "Add '%s' to Rig", currentBoneID.c_str() );
            ImGuiX::ItemTooltip( str.c_str() );
            ImGui::EndDisabled();
        }

        ImGui::PopID();

        // Draw Children
        //-------------------------------------------------------------------------

        if ( pBone->m_isExpanded )
        {
            for ( BoneInfo* pChild : pBone->m_children )
            {
                RenderSkeletonTreeRow( pChild, indentLevel + 1 );
            }
        }
    }

    void IKRigEditor::DrawRigEditorWindow( UpdateContext const& context, bool isFocused )
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            ImGui::Text( "Skeleton ID: %s", m_skeleton->GetResourcePath().GetFilenameWithoutExtension().c_str() );

            ImGui::Separator();

            EE_ASSERT( m_pSkeletonTreeRoot != nullptr );

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 0 ) );
            if ( ImGui::BeginTable( "RET", 2, 0 ) )
            {
                ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Controls", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );

                RenderSkeletonTreeRow( m_pSkeletonTreeRoot );

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
        }
    }

    void IKRigEditor::DrawRigEditorDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        if ( m_selectedBoneID.IsValid() )
        {
            IKRigResourceDescriptor* pDescriptor = GetRigDescriptor();

            int32_t const bodyIdx = GetBodyIndexForBoneID( m_selectedBoneID );
            if ( bodyIdx != InvalidIndex )
            {
                if ( m_propertyGrid.GetEditedType() != &pDescriptor->m_links[bodyIdx] )
                {
                    m_propertyGrid.SetTypeToEdit( &pDescriptor->m_links[bodyIdx] );
                }
            }
            else
            {
                if ( m_propertyGrid.GetEditedType() != nullptr )
                {
                    m_propertyGrid.SetTypeToEdit( nullptr );
                }
            }
        }
        else
        {
            if ( m_propertyGrid.GetEditedType() != nullptr )
            {
                m_propertyGrid.SetTypeToEdit( nullptr );
            }
        }

        //-------------------------------------------------------------------------

        m_propertyGrid.UpdateAndDraw();
    }

    //-------------------------------------------------------------------------

    void IKRigEditor::RequestDebugSession()
    {
        EE_ASSERT( m_debugState == DebugState::None );

        if ( IsDirty() )
        {
            Save();

            if ( !RequestImmediateResourceCompilation( m_editedResource.GetResourceID() ) )
            {
                return;
            }
        }

        m_debugState = DebugState::RequestStart;
    }

    void IKRigEditor::StartDebugging()
    {
        EE_ASSERT( m_editedResource.IsLoaded() && m_editedResource->IsValid() );
        EE_ASSERT( !IsDebugging() );
        EE_ASSERT( m_debugState == DebugState::RequestStart );

        m_pRig = EE::New<IKRig>( m_editedResource.GetPtr() );

        m_pPreviewPose = EE::New<Pose>( m_skeleton.GetPtr(), Pose::Type::ReferencePose );
        if ( m_previewAnimation.IsLoaded() )
        {
            m_previewAnimation->GetPose( m_animTime, m_pPreviewPose );
        }

        m_pFinalPose = EE::New<Pose>( m_skeleton.GetPtr(), Pose::Type::ReferencePose );
        m_debugState = DebugState::Debugging;

        CreateEffectorDebugState();
    }

    void IKRigEditor::StopDebugging()
    {
        EE_ASSERT( IsDebugging() );
        EE_ASSERT( m_debugState == DebugState::Debugging );

        DestroyEffectorGizmos();

        EE::Delete( m_pPreviewPose );
        EE::Delete( m_pFinalPose );
        EE::Delete( m_pRig );
        m_debugState = DebugState::None;

        if ( m_pPreviewMeshComponent != nullptr && m_pPreviewMeshComponent->WasInitialized() )
        {
            m_pPreviewMeshComponent->ResetPose();
            m_pPreviewMeshComponent->FinalizePose();
        }
    }

    void IKRigEditor::DrawDebugControlsWindow( UpdateContext const& context, bool isFocused )
    {
        //-------------------------------------------------------------------------
        // Animation
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Animation" ) )
        {
            ImGui::Indent();
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Preview Anim:" );
            ImGui::SameLine();

            ImGui::BeginDisabled( !m_skeleton.IsLoaded() );
            if ( m_resourceFilePicker.UpdateAndDraw() )
            {
                // If we need to change resource ID, switch IDs
                ResourceID const selectedResourceID = m_resourceFilePicker.GetResourceID();
                if ( selectedResourceID != m_previewAnimation.GetResourceID() )
                {
                    if ( m_previewAnimation.IsSet() && m_previewAnimation.WasRequested() )
                    {
                        UnloadResource( &m_previewAnimation );
                    }

                    if ( selectedResourceID.IsValid() )
                    {
                        m_previewAnimation = selectedResourceID;
                    }
                    else
                    {
                        m_previewAnimation.Clear();
                    }

                    if ( m_previewAnimation.IsSet() )
                    {
                        LoadResource( &m_previewAnimation );
                    }
                }
            }

            ImGui::Checkbox( "Enable Looping", &m_enableAnimationLooping );
            ImGui::Checkbox( "Auto Start", &m_autoPlayAnimation );

            //-------------------------------------------------------------------------

            bool const hasValidAndLoadedPreviewAnim = m_previewAnimation.IsSet() && m_previewAnimation.IsLoaded();
            ImGui::BeginDisabled( !IsDebugging() || !hasValidAndLoadedPreviewAnim );

            ImVec2 const buttonSize( 30, 24 );
            if ( m_isPlayingAnimation )
            {
                if ( ImGui::Button( EE_ICON_STOP"##StopAnim", buttonSize ) )
                {
                    m_isPlayingAnimation = false;
                }
            }
            else
            {
                if ( ImGui::Button( EE_ICON_PLAY"##PlayAnim", buttonSize ) )
                {
                    m_isPlayingAnimation = true;

                    if ( !m_enableAnimationLooping && m_animTime == 1.0f )
                    {
                        m_animTime = 0.0f;
                    }
                }
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::SetNextItemWidth( -1 );
            ImGui::BeginDisabled( !hasValidAndLoadedPreviewAnim );
            float currentAnimTime = m_animTime.ToFloat();
            if ( ImGui::SliderFloat( "##AnimTime", &currentAnimTime, 0, 1, "%.3f", ImGuiSliderFlags_NoInput ) )
            {
                m_isPlayingAnimation = false;
                m_animTime = currentAnimTime;
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::EndDisabled();
            ImGui::Unindent();
        }

        //-------------------------------------------------------------------------
        // Effectors
        //-------------------------------------------------------------------------

        if ( IsDebugging() )
        {
            ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
            if ( ImGui::CollapsingHeader( "Effectors" ) )
            {
                ImGui::Indent();

                ImVec2 const buttonSize = ImVec2( ( ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x ) / 2, 0.0f );

                IKRigResourceDescriptor const* pDescriptor = GetRigDescriptor();
                int32_t const numLinks = m_editedResource->GetNumLinks();
                int32_t effectorIdx = 0;
                for ( int32_t i = 0; i < numLinks; i++ )
                {
                    if ( !pDescriptor->m_links[i].m_isEffector )
                    {
                        continue;
                    }

                    EffectorState& effector = m_effectorDebugStates[effectorIdx];

                    ImGui::PushID( &effector );

                    ImGui::SeparatorText( pDescriptor->m_links[i].m_boneID.c_str() );

                    if ( m_selectedEffectorIndex == effectorIdx )
                    {
                        ImGuiX::IconButtonColored( EE_ICON_AXIS_ARROW, "Selected", ImGuiX::Style::s_colorAccent2, Colors::White, Colors::White, ImVec2( -1, 0 ) );
                    }
                    else
                    {
                        if ( ImGuiX::IconButton( EE_ICON_SELECT, "Select Effector", Colors::White, ImVec2( -1, 0 ) ) )
                        {
                            m_selectedEffectorIndex = effectorIdx;
                        }
                    }

                    ImGuiX::InputTransform( ( effector.m_behavior == EffectorState::Behavior::Fixed ) ? &effector.m_transform : &effector.m_offsetTransform, -1, false);

                    int32_t selectedBehavior = (int32_t) effector.m_behavior;
                    ImGui::SetNextItemWidth( buttonSize.x );
                    if ( ImGui::Combo( "##Behavior", &selectedBehavior, "Fixed\0FollowPose\0\0" ) )
                    {
                        effector.m_behavior = (EffectorState::Behavior) selectedBehavior;
                        effector.m_offsetTransform = Transform::Identity;
                    }

                    ImGui::SameLine();

                    if ( ImGuiX::IconButton( EE_ICON_RESTART, "Reset", ImGuiX::Style::s_colorText, buttonSize ) )
                    {
                        effector.m_offsetTransform = Transform::Identity;
                        effector.m_transform = m_pPreviewPose->GetModelSpaceTransform( effector.m_boneIdx );
                    }

                    ImGui::PopID();

                    effectorIdx++;
                }

                ImGui::Unindent();
            }
        }
    }

    void IKRigEditor::CreateEffectorDebugState()
    {
        EE_ASSERT( m_editedResource.IsLoaded() && m_editedResource->IsValid() );

        m_selectedEffectorIndex = InvalidIndex;
        m_effectorGizmo.SetCoordinateSystemSpace( CoordinateSpace::Local );
        m_effectorGizmo.SetMode( ImGuiX::Gizmo::GizmoMode::Translation );
        m_offsetEffectorGizmo.SetCoordinateSystemSpace( CoordinateSpace::Local );
        m_offsetEffectorGizmo.SetMode( ImGuiX::Gizmo::GizmoMode::Translation );

        IKRigResourceDescriptor const* pDescriptor = GetRigDescriptor();
        int32_t const numLinks = m_editedResource->GetNumLinks();
        for ( int32_t i = 0; i < numLinks; i++ )
        {
            if ( !pDescriptor->m_links[i].m_isEffector )
            {
                continue;
            }

            int32_t const boneIdx = m_skeleton->GetBoneIndex( pDescriptor->m_links[i].m_boneID );
            EE_ASSERT( boneIdx != InvalidIndex );

            EffectorState& effector = m_effectorDebugStates.emplace_back();
            effector.m_linkIdx = i;
            effector.m_boneIdx = boneIdx;
            effector.m_transform = m_pPreviewPose->GetModelSpaceTransform( boneIdx );
        }
    }

    void IKRigEditor::DestroyEffectorGizmos()
    {
        m_effectorDebugStates.clear();
    }
}