#include "ResourceEditor_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "Engine/Animation/Components/Component_AnimationClipPlayer.h"
#include "Engine/Animation/Systems/EntitySystem_Animation.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "Engine/Animation/AnimationPose.h"
#include "Base/Math/MathUtils.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_EDITOR_FACTORY( AnimationClipEditorFactory, AnimationClip, AnimationClipEditor );

    //-------------------------------------------------------------------------

    AnimationClipEditor::AnimationClipEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : TResourceEditor<AnimationClip>( pToolsContext, pWorld, resourceID )
        , m_eventTimeline( *pToolsContext->m_pTypeRegistry )
        , m_timelineEditor( &m_eventTimeline, [this] () { BeginDescriptorModification(); }, [this] () { EndDescriptorModification(); } )
        , m_propertyGrid( m_pToolsContext )
        , m_animationClipBrowser( pToolsContext )
    {
        m_timelineEditor.SetLooping( true );

        //-------------------------------------------------------------------------

        auto const PreDescEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction == nullptr );
            EE_ASSERT( IsDescriptorLoaded() );
            BeginDescriptorModification();
        };

        auto const PostDescEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );
            EE_ASSERT( IsDescriptorLoaded() );
            EndDescriptorModification();
        };

        m_propertyGridPreEditEventBindingID = m_propertyGrid.OnPreEdit().Bind( PreDescEdit );
        m_propertyGridPostEditEventBindingID = m_propertyGrid.OnPostEdit().Bind( PostDescEdit );
    }

    AnimationClipEditor::~AnimationClipEditor()
    {
        m_propertyGrid.OnPreEdit().Unbind( m_propertyGridPreEditEventBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_propertyGridPostEditEventBindingID );
    }

    void AnimationClipEditor::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topLeftDockID = 0, topRightDockID = 0, bottomDockID = 0, bottomLeftDockID = 0, bottomRightDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.33f, &bottomDockID, &topLeftDockID );
        ImGui::DockBuilderSplitNode( topLeftDockID, ImGuiDir_Right, 0.33f, &topRightDockID, &topLeftDockID );
        ImGui::DockBuilderSplitNode( bottomDockID, ImGuiDir_Right, 0.25f, &bottomRightDockID, &bottomLeftDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Viewport" ).c_str(), topLeftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Timeline" ).c_str(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Track Data" ).c_str(), bottomRightDockID);
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Descriptor" ).c_str(), bottomRightDockID);
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Details" ).c_str(), bottomRightDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Clip Browser" ).c_str(), topRightDockID);
    }

    //-------------------------------------------------------------------------

    void AnimationClipEditor::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TResourceEditor<AnimationClip>::Initialize( context );

        m_characterPoseUpdateRequested = true;

        //-------------------------------------------------------------------------

        CreateToolWindow( "Timeline", [this] ( UpdateContext const& context, bool isFocused ) { DrawTimelineWindow( context, isFocused ); }, ImVec2( 0, 0 ), true );
        CreateToolWindow( "Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawDetailsWindow( context, isFocused ); } );
        CreateToolWindow( "Clip Browser", [this] ( UpdateContext const& context, bool isFocused ) { DrawClipBrowser( context, isFocused ); } );
        CreateToolWindow( "Bone Hierarchy", [this] ( UpdateContext const& context, bool isFocused ) { DrawHierarchyWindow( context, isFocused ); } );
    }

    void AnimationClipEditor::Shutdown( UpdateContext const& context )
    {
        // No need to call destroy as the entity will be destroyed as part of the world shutdown
        m_pPreviewEntity = nullptr;
        m_pAnimationComponent = nullptr;
        m_pMeshComponent = nullptr;

        TResourceEditor<AnimationClip>::Shutdown( context );
    }

    void AnimationClipEditor::CreatePreviewEntity()
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );
        EE_ASSERT( m_editedResource.IsLoaded() );

        // Create animation component
        m_pAnimationComponent = EE::New<AnimationClipPlayerComponent>( StringID( "Animation Component" ) );
        m_pAnimationComponent->SetAnimation( m_editedResource.GetResourceID() );
        m_pAnimationComponent->SetPlayMode( AnimationClipPlayerComponent::PlayMode::Posed );
        m_pAnimationComponent->SetSkeletonLOD( m_skeletonLOD );

        // Create entity
        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        m_pPreviewEntity->CreateSystem<AnimationSystem>();
        m_pPreviewEntity->AddComponent( m_pAnimationComponent );

        // Create the primary mesh component
        auto pPrimarySkeleton = m_editedResource->GetSkeleton();
        if ( pPrimarySkeleton->GetPreviewMeshID().IsValid() )
        {
            m_pMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Primary Mesh Component" ) );
            m_pMeshComponent->SetSkeleton( pPrimarySkeleton->GetResourceID() );
            m_pMeshComponent->SetMesh( pPrimarySkeleton->GetPreviewMeshID() );
            m_pMeshComponent->SetWorldTransform( m_characterTransform );
            m_pMeshComponent->SetVisible( m_showMesh );
            m_pPreviewEntity->AddComponent( m_pMeshComponent );

            // If we have secondary anims then create the secondary preview components
            m_secondarySkeletonAttachmentSocketIDs.clear();
            for ( auto pSecondaryAnimation : m_editedResource->GetSecondaryAnimations() )
            {
                Skeleton const* pSecondarySkeleton = pSecondaryAnimation->GetSkeleton();
                if ( pSecondarySkeleton->GetPreviewMeshID().IsValid() )
                {
                    InlineString const secondaryComponentName( InlineString::CtorSprintf(), "Secondary Mesh Component (%s)", pSecondarySkeleton->GetFriendlyName() );
                    auto pSecondaryMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( secondaryComponentName.c_str() ) );
                    pSecondaryMeshComponent->SetSkeleton( pSecondarySkeleton->GetResourceID() );
                    pSecondaryMeshComponent->SetMesh( pSecondarySkeleton->GetPreviewMeshID() );

                    // Set attachment info and add to the entity
                    pSecondaryMeshComponent->SetWorldTransform( Transform::Identity );
                    pSecondaryMeshComponent->SetAttachmentSocketID( pSecondarySkeleton->GetPreviewAttachmentSocketID() );
                    m_secondarySkeletonAttachmentSocketIDs.emplace_back( pSecondarySkeleton->GetPreviewAttachmentSocketID() );
                    m_pPreviewEntity->AddComponent( pSecondaryMeshComponent, m_pMeshComponent->GetID() );
                }
            }
        }

        //-------------------------------------------------------------------------

        AddEntityToWorld( m_pPreviewEntity );
    }

    void AnimationClipEditor::DestroyPreviewEntity()
    {
        EE_ASSERT( m_pPreviewEntity != nullptr );
        DestroyEntityInWorld( m_pPreviewEntity );
        m_pPreviewEntity = nullptr;
        m_pAnimationComponent = nullptr;
        m_pMeshComponent = nullptr;
        m_secondarySkeletonAttachmentSocketIDs.clear();
    }

    void AnimationClipEditor::OnDescriptorUnload()
    {
        m_propertyGrid.SetTypeToEdit( nullptr );
        m_timelineEditor.SetLength( 0.0f );
        m_animationClipBrowser.SetSkeleton( ResourceID() );
    }

    void AnimationClipEditor::OnDescriptorLoadCompleted()
    {
        m_characterPoseUpdateRequested = true;
        if ( IsDescriptorLoaded() )
        {
            m_animationClipBrowser.SetSkeleton( GetDescriptor<AnimationClipResourceDescriptor>()->m_skeleton.GetResourceID() );
        }
    }

    void AnimationClipEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource && m_editedResource.IsLoaded() )
        {
            m_timelineEditor.SetLength( (float) m_editedResource->GetNumFrames() - 1 );
            m_timelineEditor.SetTimeUnitConversionFactor( m_editedResource->IsSingleFrameAnimation() ? 30 : m_editedResource->GetFPS() );
            CreatePreviewEntity();
            CreateSkeletonTree();
        }
    }

    void AnimationClipEditor::OnResourceUnload( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource )
        {
            if ( m_pPreviewEntity != nullptr )
            {
                DestroyPreviewEntity();
            }

            DestroySkeletonTree();
        }
    }

    //-------------------------------------------------------------------------

    void AnimationClipEditor::PreWorldUpdate( EntityWorldUpdateContext const& updateContext )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( updateContext.GetUpdateStage() == UpdateStage::FrameEnd )
        {
            // Draw root motion in viewport
            //-------------------------------------------------------------------------

            auto drawingCtx = GetDrawingContext();
            m_editedResource->GetRootMotion().DrawDebug( drawingCtx, Transform::Identity );

            if ( m_isPoseDrawingEnabled && m_pAnimationComponent != nullptr )
            {
                // Draw the main pose
                Pose const* pPose = m_pAnimationComponent->GetPrimaryPose();
                if ( pPose != nullptr )
                {
                    if ( !m_isolateSelectedBones || m_selectedBoneIDs.empty() )
                    {
                        pPose->DrawDebug( drawingCtx, m_characterTransform, m_skeletonLOD, Colors::HotPink, 3.0f, m_shouldDrawBoneNames, nullptr );
                    }
                    else
                    {
                        TVector<int32_t> boneFilter;
                        for ( StringID selectedBoneID : m_selectedBoneIDs )
                        {
                            boneFilter.emplace_back( pPose->GetSkeleton()->GetBoneIndex( selectedBoneID ) );
                        }

                        pPose->DrawDebug( drawingCtx, m_characterTransform, m_skeletonLOD, Colors::HotPink, 3.0f, m_shouldDrawBoneNames, nullptr, boneFilter );
                    }
                }

                // Draw the secondary animation poses
                int32_t const numSecondaryPoses = m_pAnimationComponent->GetNumSecondaryPoses();
                for ( int32_t i = 0; i < numSecondaryPoses; i++ )
                {
                    Pose const* pSecondaryPose = m_pAnimationComponent->GetSecondaryPoses()[i];
                    Transform const secondaryPoseTransform = m_secondarySkeletonAttachmentSocketIDs.empty() ? m_characterTransform : m_pMeshComponent->GetAttachmentSocketTransform( m_secondarySkeletonAttachmentSocketIDs[i] );
                    pSecondaryPose->DrawDebug( drawingCtx, secondaryPoseTransform, m_skeletonLOD, Colors::Cyan, 3.0f, m_shouldDrawBoneNames, nullptr );
                }
            }

            // Draw capsule
            //-------------------------------------------------------------------------

            if ( m_isPreviewCapsuleDrawingEnabled )
            {
                Transform capsuleTransform = m_characterTransform;
                capsuleTransform.AddTranslation( capsuleTransform.GetAxisZ() * ( m_previewCapsuleHalfHeight + m_previewCapsuleRadius ) );
                drawingCtx.DrawCapsule( capsuleTransform, m_previewCapsuleRadius, m_previewCapsuleHalfHeight, Colors::LimeGreen, 3.0f );
            }
        }
    }

    void AnimationClipEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        // Update
        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            // Update pose and position
            //-------------------------------------------------------------------------

            Percentage const percentageThroughAnimation = m_timelineEditor.GetPlayheadTimeAsPercentage();
            if ( m_currentAnimTime != percentageThroughAnimation || m_characterPoseUpdateRequested )
            {
                m_currentAnimTime = percentageThroughAnimation;
                m_pAnimationComponent->SetAnimTime( percentageThroughAnimation );
                m_pAnimationComponent->SetSkeletonLOD( m_skeletonLOD );
                m_characterTransform = m_isRootMotionEnabled ? m_editedResource->GetRootTransform( percentageThroughAnimation ) : Transform::Identity;

                // Update character preview position
                if ( m_pMeshComponent != nullptr )
                {
                    m_pMeshComponent->SetWorldTransform( m_characterTransform );
                }

                m_characterPoseUpdateRequested = false;
            }
        }
    }

    void AnimationClipEditor::DrawMenu( UpdateContext const& context )
    {
        TResourceEditor<AnimationClip>::DrawMenu( context );

        // Options
        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( EE_ICON_TUNE" Options" ) )
        {
            ImGui::BeginDisabled( m_pMeshComponent == nullptr );
            if ( ImGui::Checkbox( "Show Mesh", &m_showMesh ) )
            {
                m_pMeshComponent->SetVisible( m_showMesh );
            }
            ImGui::EndDisabled();

            if ( ImGui::Checkbox( "Show Floor", &m_showFloor ) )
            {
                SetFloorVisibility( m_showFloor );
            }

            ImGui::Checkbox( "Root Motion Enabled", &m_isRootMotionEnabled );

            ImGui::Checkbox( "Draw Bone Pose", &m_isPoseDrawingEnabled );

            ImGui::Checkbox( "Draw Bone Names", &m_shouldDrawBoneNames );

            bool isUsingLowLOD = m_skeletonLOD == Skeleton::LOD::Low;
            if ( ImGui::Checkbox( "Use Low Skeleton LOD", &isUsingLowLOD ) )
            {
                m_skeletonLOD = isUsingLowLOD ? Skeleton::LOD::Low : Skeleton::LOD::High;
            }

            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Capsule Debug" );

            ImGui::Checkbox( "Show Preview Capsule", &m_isPreviewCapsuleDrawingEnabled );

            ImGui::Text( "Half-Height" );
            ImGui::SameLine( 90 );
            if ( ImGui::InputFloat( "##HH", &m_previewCapsuleHalfHeight, 0.05f ) )
            {
                m_previewCapsuleHalfHeight = Math::Clamp( m_previewCapsuleHalfHeight, 0.05f, 10.0f );
            }

            ImGui::Text( "Radius" );
            ImGui::SameLine( 90 );
            if ( ImGui::InputFloat( "##R", &m_previewCapsuleRadius, 0.01f ) )
            {
                m_previewCapsuleRadius = Math::Clamp( m_previewCapsuleRadius, 0.01f, 5.0f );
            }

            ImGui::EndMenu();
        }
    }

    void AnimationClipEditor::DrawHelpMenu() const
    {
        DrawHelpTextRow( "Navigate Timeline", "Left/Right arrows" );
        DrawHelpTextRow( "Create Event", "Enter (with track selected)" );
    }

    void AnimationClipEditor::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        TResourceEditor<AnimationClip>::DrawViewportToolbar( context, pViewport );

        //-------------------------------------------------------------------------

        if ( !IsResourceLoaded() )
        {
            return;
        }

        // Clip Info
        //-------------------------------------------------------------------------

        ImGui::Indent();

        auto PrintAnimDetails = [this] ( Color color )
        {
            Percentage const currentTime = m_timelineEditor.GetPlayheadTimeAsPercentage();
            uint32_t const numFrames = m_editedResource->GetNumFrames();
            FrameTime const frameTime = m_editedResource->GetFrameTime( currentTime );

            ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold, color );
            ImGui::Text( "Avg Linear Velocity: %.2f m/s", m_editedResource->GetAverageLinearVelocity() );
            ImGui::Text( "Avg Angular Velocity: %.2f r/s", m_editedResource->GetAverageAngularVelocity().ToFloat() );
            ImGui::Text( "Distance Covered: %.2fm", m_editedResource->GetTotalRootMotionDelta().GetTranslation().GetLength3() );
            ImGui::Text( "Frame: %.2f/%d", frameTime.ToFloat(), numFrames - 1 );
            ImGui::Text( "Time: %.2fs/%0.2fs", m_timelineEditor.GetPlayheadTimeAsPercentage().ToFloat() * m_editedResource->GetDuration(), m_editedResource->GetDuration().ToFloat() );
            ImGui::Text( "Percentage: %.2f%%", m_timelineEditor.GetPlayheadTimeAsPercentage().ToFloat() * 100 );

            if ( m_editedResource->IsAdditive() )
            {
                ImGui::Text( "Additive" );
            }
        };

        ImVec2 const cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos( cursorPos + ImVec2( 0, 1 ) );
        ImGui::Indent( 1 );
        PrintAnimDetails( Colors::Black );
        ImGui::Unindent( 1 );
        ImGui::SetCursorPos( cursorPos );
        PrintAnimDetails( Colors::Yellow );

        ImGui::Unindent();
    }

    void AnimationClipEditor::DrawTimelineWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsWaitingForResource() )
        {
            ImGui::Text( "Loading:" );
            ImGui::SameLine();
            ImGuiX::DrawSpinner( "Loading" );
        }
        else if ( HasLoadingFailed() )
        {
            ImGui::Text( "Loading Failed: %s", m_editedResource.GetResourceID().c_str() );
        }
        else
        {
            // Track editor and property grid
            //-------------------------------------------------------------------------

            m_timelineEditor.UpdateAndDraw( GetEntityWorld()->GetTimeScale() * context.GetDeltaTime() );

            // Handle selection changes
            auto const& selectedItems = m_timelineEditor.GetSelectedItems();
            if ( !selectedItems.empty() )
            {
                if ( selectedItems.back()->GetData() != m_propertyGrid.GetEditedType() )
                {
                    m_propertyGrid.SetTypeToEdit( selectedItems.back()->GetData() );
                }
            }
            else // Clear property grid
            {
                if ( m_propertyGrid.GetEditedType() != nullptr )
                {
                    m_propertyGrid.SetTypeToEdit( nullptr );
                }
            }
        }
    }

    void AnimationClipEditor::DrawHierarchyWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsResourceLoaded() )
        {
            ImGui::Text( "Nothing to show!" );
            return;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::Button( EE_ICON_SKULL" Open Skeleton", ImVec2( -1, 0 ) ) )
        {
            Skeleton const* pSkeleton = m_editedResource.GetPtr()->GetSkeleton();
            m_pToolsContext->TryOpenResource( pSkeleton->GetResourceID() );
        }

        //-------------------------------------------------------------------------

        ImGuiX::Checkbox( "Draw Bone Names", &m_shouldDrawBoneNames );
        ImGui::SameLine();
        ImGuiX::Checkbox( "Isolate Selected Bones", &m_isolateSelectedBones );
        ImGui::SameLine();
        ImGuiX::Checkbox( "Show Transforms", &m_showHierarchyTransforms );

        //-------------------------------------------------------------------------

        if ( ImGui::BeginChild( "Table", ImVec2( ImGui::GetContentRegionAvail().x, -1 ), ImGuiChildFlags_AutoResizeY ) )
        {
            static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
            if ( ImGui::BeginTable( "BoneTreeTable", m_showHierarchyTransforms ? 3 : 1, flags ) )
            {
                // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
                ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );

                if ( m_showHierarchyTransforms )
                {
                    ImGui::TableSetupColumn( "Bone Space", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "World Space", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
                }

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                DrawSkeletonTreeRow( m_pSkeletonTreeRoot );

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }

    void AnimationClipEditor::DrawDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        m_propertyGrid.DrawGrid();
    }

    void AnimationClipEditor::DrawClipBrowser( UpdateContext const& context, bool isFocused )
    {
        m_animationClipBrowser.Draw( context, isFocused );
    }

    //-------------------------------------------------------------------------

    void AnimationClipEditor::CreateSkeletonTree()
    {
        TVector<BoneInfo*> boneInfos;

        // Create all infos
        Skeleton const* pSkeleton = m_editedResource.GetPtr()->GetSkeleton();
        int32_t const numBones = pSkeleton->GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& pBoneInfo = boneInfos.emplace_back( EE::New<BoneInfo>() );
            pBoneInfo->m_boneIdx = i;
        }

        // Create hierarchy
        for ( auto i = 1; i < numBones; i++ )
        {
            int32_t const parentBoneIdx = pSkeleton->GetParentBoneIndex( i );
            EE_ASSERT( parentBoneIdx != InvalidIndex );
            boneInfos[parentBoneIdx]->m_children.emplace_back( boneInfos[i] );
        }

        // Set root
        m_pSkeletonTreeRoot = boneInfos[0];
    }

    void AnimationClipEditor::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    void AnimationClipEditor::DrawSkeletonTreeRow( BoneInfo* pBone )
    {
        EE_ASSERT( pBone != nullptr );

        Skeleton const* pSkeleton = m_editedResource.GetPtr()->GetSkeleton();
        int32_t const boneIdx = pBone->m_boneIdx;
        StringID const currentBoneID = pSkeleton->GetBoneID( boneIdx );
        bool const isSelected = VectorContains( m_selectedBoneIDs, currentBoneID );

        Color rowColor = ImGuiX::Style::s_colorText;

        if ( isSelected )
        {
            rowColor = ImGuiX::Style::s_colorAccent0;
        }

        //-------------------------------------------------------------------------

        ImGui::TableNextRow();

        //-------------------------------------------------------------------------
        // Draw Label
        //-------------------------------------------------------------------------

        bool const isHighLOD = pSkeleton->IsBoneHighLOD( boneIdx );
        InlineString const boneLabel( InlineString::CtorSprintf(), "%d. %s", boneIdx, currentBoneID.c_str() );

        ImGui::TableNextColumn();

        int32_t treeNodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        if ( isSelected )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::SetNextItemOpen( pBone->m_isExpanded );
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor( ImGuiCol_Text, rowColor );
        pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );

        // Handle bone selection
        if ( ImGui::IsItemClicked() )
        {
            // Multi-select
            if ( ImGui::IsKeyDown( ImGuiMod_Ctrl ) )
            {
                // Remove from selection
                if ( VectorContains( m_selectedBoneIDs, currentBoneID ) )
                {
                    m_selectedBoneIDs.erase_first( currentBoneID );
                }
                else // Add to selection
                {
                    m_selectedBoneIDs.emplace_back( currentBoneID );
                }
            }
            else // Single selection
            {
                m_selectedBoneIDs.clear();
                m_selectedBoneIDs.emplace_back( currentBoneID );
            }
        }

        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------
        // Draw Transforms
        //-------------------------------------------------------------------------

        if ( m_showHierarchyTransforms )
        {
            Pose const* pPose = m_pAnimationComponent->GetPrimaryPose();

            ImGui::TableNextColumn();

            if ( pPose != nullptr )
            {
                Transform boneTransform = ( boneIdx == 0 ) ? m_editedResource->GetRootTransform( m_pAnimationComponent->GetAnimTime() ) : pPose->GetTransform( boneIdx );
                ImGuiX::DrawTransform( boneTransform );
            }

            ImGui::TableNextColumn();

            if ( pPose != nullptr && boneIdx != 0 )
            {
                Transform boneTransform = pPose->GetGlobalTransform( boneIdx );
                ImGuiX::DrawTransform( boneTransform );
            }
        }

        //-------------------------------------------------------------------------
        // Draw Context Menu
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
        // Draw Children
        //-------------------------------------------------------------------------

        if ( pBone->m_isExpanded )
        {
            for ( BoneInfo* pChildBone : pBone->m_children )
            {
                DrawSkeletonTreeRow( pChildBone );
            }
            ImGui::TreePop();
        }
    }

    //-------------------------------------------------------------------------

    bool AnimationClipEditor::SaveData()
    {
        auto const status = m_timelineEditor.GetTrackContainerValidationStatus();
        if ( status == Timeline::Track::Status::HasErrors )
        {
            pfd::message( "Invalid Events", "This animation clip has one or more invalid event tracks. These events will not be available in the game!", pfd::choice::ok, pfd::icon::error ).result();
        }

        if ( !TResourceEditor<AnimationClip>::SaveData() )
        {
            return false;
        }

        m_propertyGrid.ClearDirty();

        return true;
    }

    void AnimationClipEditor::ReadCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& descriptorObjectValue )
    {
        m_eventTimeline.Serialize( typeRegistry, descriptorObjectValue );
    }

    void AnimationClipEditor::WriteCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer )
    {
        static_cast<Timeline::TimelineData&>( m_eventTimeline ).Serialize( typeRegistry, writer );
    }

    void AnimationClipEditor::PreUndoRedo( UndoStack::Operation operation )
    {
        TResourceEditor<AnimationClip>::PreUndoRedo( operation );
        m_propertyGrid.SetTypeToEdit( nullptr );
    }
}