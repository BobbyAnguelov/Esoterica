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
        CreateToolWindow( "Track Data", [this] ( UpdateContext const& context, bool isFocused ) { DrawTrackDataWindow( context, isFocused ); } );
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
                    TBitFlags<Pose::DrawFlags> drawFlags;
                    if ( m_shouldDrawBoneNames )
                    {
                        drawFlags.SetFlag( Pose::DrawFlags::DrawBoneNames );
                    }

                    pPose->DrawDebug( drawingCtx, m_characterTransform, Colors::HotPink, 3.0f, nullptr, drawFlags );
                }

                // Draw the secondary animation poses
                int32_t const numSecondaryPoses = m_pAnimationComponent->GetNumSecondaryPoses();
                for ( int32_t i = 0; i < numSecondaryPoses; i++ )
                {
                    Pose const* pSecondaryPose = m_pAnimationComponent->GetSecondaryPoses()[i];
                    Transform const secondaryPoseTransform = m_secondarySkeletonAttachmentSocketIDs.empty() ? m_characterTransform : m_pMeshComponent->GetAttachmentSocketTransform( m_secondarySkeletonAttachmentSocketIDs[i] );
                    pSecondaryPose->DrawDebug( drawingCtx, secondaryPoseTransform, Colors::Cyan, 3.0f, nullptr );
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

    void AnimationClipEditor::DrawTrackDataWindow( UpdateContext const& context, bool isFocused )
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
            // There may be a frame delay between the UI and the entity system creating the pose
            Pose const* pPose = m_pAnimationComponent->GetPrimaryPose();
            if ( pPose != nullptr )
            {
                if ( ImGui::BeginTable( "TrackDataTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg ) )
                {
                    ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "Transform", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableHeadersRow();

                    //-------------------------------------------------------------------------

                    Skeleton const* pSkeleton = pPose->GetSkeleton();
                    int32_t const numBones = pSkeleton->GetNumBones();

                    ImGuiListClipper clipper;
                    clipper.Begin( numBones );
                    while ( clipper.Step() )
                    {
                        for ( int boneIdx = clipper.DisplayStart; boneIdx < clipper.DisplayEnd; boneIdx++ )
                        {
                            Transform const& boneTransform = ( boneIdx == 0 ) ? m_editedResource->GetRootTransform( m_pAnimationComponent->GetAnimTime() ) : pPose->GetGlobalTransform( boneIdx );

                            ImGui::TableNextColumn();
                            ImGui::Text( "%d. %s", boneIdx, pSkeleton->GetBoneID( boneIdx ).c_str() );

                            ImGui::TableNextColumn();
                            DrawTransform( boneTransform );
                        }
                    }
                    clipper.End();

                    ImGui::EndTable();
                }
            }
        }
        else
        {
            ImGui::Text( "Nothing to show!" );
        }
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