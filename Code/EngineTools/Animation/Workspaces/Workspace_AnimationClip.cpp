#include "Workspace_AnimationClip.h"
#include "EngineTools/Animation/Events/AnimationEventEditor.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "Engine/Animation/Components/Component_AnimationClipPlayer.h"
#include "Engine/Animation/Systems/EntitySystem_Animation.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "Engine/Animation/AnimationPose.h"
#include "System/Math/MathUtils.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_WORKSPACE_FACTORY( AnimationClipWorkspaceFactory, AnimationClip, AnimationClipWorkspace );

    //-------------------------------------------------------------------------

    AnimationClipWorkspace::AnimationClipWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : TWorkspace<AnimationClip>( pToolsContext, pWorld, resourceID )
        , m_eventEditor( *pToolsContext->m_pTypeRegistry )
        , m_propertyGrid( m_pToolsContext )
    {
        auto OnBeginMod = [this]( Timeline::TrackContainer* pContainer )
        {
            if ( pContainer != m_eventEditor.GetTrackContainer() )
            {
                return;
            }

            BeginDescriptorModification();
        };

        auto OnEndMod = [this]( Timeline::TrackContainer* pContainer )
        {
            if ( pContainer != m_eventEditor.GetTrackContainer() )
            {
                return;
            }

            EndDescriptorModification();
        };

        m_eventEditor.SetLooping( true );

        m_beginModEventID = Timeline::TrackContainer::s_onBeginModification.Bind( OnBeginMod );
        m_endModEventID = Timeline::TrackContainer::s_onEndModification.Bind( OnEndMod );

        //-------------------------------------------------------------------------

        auto const PreDescEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction == nullptr );
            EE_ASSERT( IsADescriptorWorkspace() && IsDescriptorLoaded() );
            BeginDescriptorModification();
        };

        auto const PostDescEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );
            EE_ASSERT( IsADescriptorWorkspace() && IsDescriptorLoaded() );
            EndDescriptorModification();
        };

        m_propertyGridPreEditEventBindingID = m_propertyGrid.OnPreEdit().Bind( PreDescEdit );
        m_propertyGridPostEditEventBindingID = m_propertyGrid.OnPostEdit().Bind( PostDescEdit );
    }

    AnimationClipWorkspace::~AnimationClipWorkspace()
    {
        Timeline::TrackContainer::s_onBeginModification.Unbind( m_beginModEventID );
        Timeline::TrackContainer::s_onEndModification.Unbind( m_endModEventID );

        m_propertyGrid.OnPreEdit().Unbind( m_propertyGridPreEditEventBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_propertyGridPostEditEventBindingID );
    }

    void AnimationClipWorkspace::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
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

    void AnimationClipWorkspace::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TWorkspace<AnimationClip>::Initialize( context );

        if ( m_pDescriptor != nullptr )
        {
            CreatePreviewEntity();
        }

        m_characterPoseUpdateRequested = true;

        //-------------------------------------------------------------------------

        CreateToolWindow( "Timeline", [this] ( UpdateContext const& context, bool isFocused ) { DrawTimelineWindow( context, isFocused ); }, ImVec2( 0, 0 ) );
        CreateToolWindow( "Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawDetailsWindow( context, isFocused ); } );
        CreateToolWindow( "Clip Browser", [this] ( UpdateContext const& context, bool isFocused ) { DrawClipBrowser( context, isFocused ); } );
        CreateToolWindow( "Track Data", [this] ( UpdateContext const& context, bool isFocused ) { DrawTrackDataWindow( context, isFocused ); } );
    }

    void AnimationClipWorkspace::Shutdown( UpdateContext const& context )
    {
        // No need to call destroy as the entity will be destroyed as part of the world shutdown
        m_pPreviewEntity = nullptr;
        m_pAnimationComponent = nullptr;
        m_pMeshComponent = nullptr;

        TWorkspace<AnimationClip>::Shutdown( context );
    }

    void AnimationClipWorkspace::CreatePreviewEntity()
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        // Create animation component
        m_pAnimationComponent = EE::New<AnimationClipPlayerComponent>( StringID( "Animation Component" ) );
        m_pAnimationComponent->SetAnimation( m_workspaceResource.GetResourceID() );
        m_pAnimationComponent->SetPlayMode( AnimationClipPlayerComponent::PlayMode::Posed );

        // Create entity
        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        m_pPreviewEntity->CreateSystem<AnimationSystem>();
        m_pPreviewEntity->AddComponent( m_pAnimationComponent );

        // Create the mesh component
        CreatePreviewMeshComponent();

        //-------------------------------------------------------------------------

        AddEntityToWorld( m_pPreviewEntity );
    }

    void AnimationClipWorkspace::DestroyPreviewEntity()
    {
        EE_ASSERT( m_pPreviewEntity != nullptr );
        DestroyEntityInWorld( m_pPreviewEntity );
        m_pPreviewEntity = nullptr;
        m_pAnimationComponent = nullptr;
        m_pMeshComponent = nullptr;
    }

    void AnimationClipWorkspace::CreatePreviewMeshComponent()
    {
        if ( m_pPreviewEntity == nullptr )
        {
            return;
        }

        // Load resource descriptor for skeleton to get the preview mesh
        auto pAnimClipDescriptor = GetDescriptor<AnimationClipResourceDescriptor>();
        if ( pAnimClipDescriptor->m_skeleton.IsSet() )
        {
            FileSystem::Path const resourceDescPath = GetFileSystemPath( pAnimClipDescriptor->m_skeleton.GetResourcePath() );
            SkeletonResourceDescriptor resourceDesc;
            if ( Resource::ResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, resourceDescPath, resourceDesc ) && resourceDesc.m_previewMesh.IsSet() )
            {
                // Create a preview mesh component
                m_pMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
                m_pMeshComponent->SetSkeleton( pAnimClipDescriptor->m_skeleton.GetResourceID() );
                if ( resourceDesc.m_previewMesh.IsSet() )
                {
                    m_pMeshComponent->SetMesh( resourceDesc.m_previewMesh.GetResourceID() );
                }

                m_pMeshComponent->SetWorldTransform( m_characterTransform );
            }
        }

        if ( m_pMeshComponent != nullptr )
        {
            m_pPreviewEntity->AddComponent( m_pMeshComponent );
        }
    }

    void AnimationClipWorkspace::DestroyPreviewMeshComponent()
    {
        EE_ASSERT( m_pPreviewEntity != nullptr && m_pMeshComponent != nullptr );
        m_pPreviewEntity->DestroyComponent( m_pMeshComponent->GetID() );
        m_pMeshComponent = nullptr;
    }

    void AnimationClipWorkspace::OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded )
    {
        TWorkspace<AnimationClip>::OnHotReloadStarted( descriptorNeedsReload, resourcesToBeReloaded );

        // If someone messed with this resource outside of this editor
        if ( descriptorNeedsReload )
        {
            m_propertyGrid.SetTypeToEdit( nullptr );
            if ( m_pPreviewEntity != nullptr )
            {
                DestroyPreviewEntity();
            }
        }

        if ( m_pMeshComponent != nullptr )
        {
            DestroyPreviewMeshComponent();
        }
    }

    void AnimationClipWorkspace::OnHotReloadComplete()
    {
        TWorkspace<AnimationClip>::OnHotReloadComplete();

        if ( m_pPreviewEntity == nullptr )
        {
            CreatePreviewEntity();
        }

        if ( m_pPreviewEntity != nullptr && m_pMeshComponent == nullptr )
        {
            CreatePreviewMeshComponent();
        }

        m_characterPoseUpdateRequested = true;
    }

    //-------------------------------------------------------------------------

    void AnimationClipWorkspace::Update( UpdateContext const& context, bool isFocused )
    {
        if ( IsResourceLoaded() )
        {
            m_eventEditor.SetAnimationInfo( m_workspaceResource->GetNumFrames(), m_workspaceResource->GetFPS() );
        }

        if ( m_eventEditor.IsDirty() )
        {
            MarkDirty();
        }

        // Enable the global timeline keyboard shortcuts
        if ( isFocused && !m_isDescriptorWindowFocused && !m_isDetailsWindowFocused )
        {
            m_eventEditor.HandleGlobalKeyboardInputs();
        }

        // Update
        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            // Update pose and position
            //-------------------------------------------------------------------------

            Percentage const percentageThroughAnimation = m_eventEditor.GetCurrentTimeAsPercentage();
            if ( m_currentAnimTime != percentageThroughAnimation || m_characterPoseUpdateRequested )
            {
                m_currentAnimTime = percentageThroughAnimation;
                m_pAnimationComponent->SetAnimTime( percentageThroughAnimation );
                m_characterTransform = m_isRootMotionEnabled ? m_workspaceResource->GetRootTransform( percentageThroughAnimation ) : Transform::Identity;

                // Update character preview position
                if ( m_pMeshComponent != nullptr )
                {
                    m_pMeshComponent->SetWorldTransform( m_characterTransform );
                }

                m_characterPoseUpdateRequested = false;
            }
            
            // Draw root motion in viewport
            //-------------------------------------------------------------------------

            auto drawingCtx = GetDrawingContext();
            m_workspaceResource->GetRootMotion().DrawDebug( drawingCtx, Transform::Identity );

            if ( m_isPoseDrawingEnabled && m_pAnimationComponent != nullptr )
            {
                Pose const* pPose = m_pAnimationComponent->GetPose();
                if ( pPose != nullptr )
                {
                    drawingCtx.Draw( *pPose, m_characterTransform );
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

    void AnimationClipWorkspace::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        TWorkspace<AnimationClip>::DrawViewportToolbar( context, pViewport );

        //-------------------------------------------------------------------------

        if ( !IsResourceLoaded() )
        {
            return;
        }

        // Debug Options
        //-------------------------------------------------------------------------

        auto DrawDebugOptions = [this] ()
        {
            ImGuiX::TextSeparator( "Visualization" );

            ImGui::Checkbox( "Root Motion Enabled", &m_isRootMotionEnabled );

            ImGui::Checkbox( "Draw Bone Pose", &m_isPoseDrawingEnabled );

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Capsule Debug" );

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
        };

        ImGui::SameLine();
        DrawViewportToolbarComboIcon( "##DebugOptions", EE_ICON_BUG_CHECK, "Debug Options", DrawDebugOptions );

        // Play Button
        //-------------------------------------------------------------------------

        ImGui::SameLine();

        if ( m_eventEditor.IsPlaying() )
        {
            if ( ImGuiX::IconButton( EE_ICON_PAUSE, "Pause", ImGuiX::ImColors::Yellow, ImVec2( 70, 0 ) ) )
            {
                m_eventEditor.Pause();
            }
        }
        else
        {
            if ( ImGuiX::IconButton( EE_ICON_PLAY, "Play", ImGuiX::ImColors::Lime, ImVec2( 70, 0 ) ) )
            {
                m_eventEditor.Play();
            }
        }

        // Clip Info
        //-------------------------------------------------------------------------

        ImGui::Indent();

        auto PrintAnimDetails = [this] ( Color color )
        {
            Percentage const currentTime = m_eventEditor.GetCurrentTimeAsPercentage();
            uint32_t const numFrames = m_workspaceResource->GetNumFrames();
            FrameTime const frameTime = m_workspaceResource->GetFrameTime( currentTime );

            ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold, color );
            ImGui::Text( "Avg Linear Velocity: %.2f m/s", m_workspaceResource->GetAverageLinearVelocity() );
            ImGui::Text( "Avg Angular Velocity: %.2f r/s", m_workspaceResource->GetAverageAngularVelocity().ToFloat() );
            ImGui::Text( "Distance Covered: %.2fm", m_workspaceResource->GetTotalRootMotionDelta().GetTranslation().GetLength3() );
            ImGui::Text( "Frame: %.2f/%d (%.2f/%d)", frameTime.ToFloat(), numFrames - 1, frameTime.ToFloat() + 1.0f, numFrames ); // Draw offset time too to match DCC timelines that start at 1
            ImGui::Text( "Time: %.2fs/%0.2fs", m_eventEditor.GetCurrentTimeAsPercentage().ToFloat() * m_workspaceResource->GetDuration(), m_workspaceResource->GetDuration().ToFloat() );
            ImGui::Text( "Percentage: %.2f%%", m_eventEditor.GetCurrentTimeAsPercentage().ToFloat() * 100 );

            if ( m_workspaceResource->IsAdditive() )
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

    void AnimationClipWorkspace::DrawTimelineWindow( UpdateContext const& context, bool isFocused )
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
        else
        {
            // Track editor and property grid
            //-------------------------------------------------------------------------

            m_eventEditor.UpdateAndDraw( GetWorld()->GetTimeScale() * context.GetDeltaTime() );

            // Transfer dirty state from property grid
            if ( m_propertyGrid.IsDirty() )
            {
                m_eventEditor.MarkDirty();
            }

            // Handle selection changes
            auto const& selectedItems = m_eventEditor.GetSelectedItems();
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

    void AnimationClipWorkspace::DrawTrackDataWindow( UpdateContext const& context, bool isFocused )
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
            Pose const* pPose = m_pAnimationComponent->GetPose();
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
                            Transform const& boneTransform = ( boneIdx == 0 ) ? m_workspaceResource->GetRootTransform( m_pAnimationComponent->GetAnimTime() ) : pPose->GetGlobalTransform( boneIdx );

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

    void AnimationClipWorkspace::DrawDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        m_propertyGrid.DrawGrid();
        m_isDetailsWindowFocused = true;
    }

    void AnimationClipWorkspace::DrawClipBrowser( UpdateContext const& context, bool isFocused )
    {
        auto ApplyClipFilter = [this] ()
        {
            m_filteredClips.clear();

            for ( auto const& clip : m_clipsWithSameSkeleton )
            {
                if ( m_clipBrowserFilter.MatchesFilter( clip.c_str() ) )
                {
                    m_filteredClips.emplace_back( clip );
                }
            }
        };

        // Update Cache
        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            if ( m_clipBrowserCacheRefreshTimer.GetElapsedTimeSeconds() > 1.0f )
            {
                auto SkeletonFilter = [this] ( Resource::ResourceDescriptor const* pDescriptor )
                {
                    auto pClipDescriptor = Cast<AnimationClipResourceDescriptor>( pDescriptor );
                    return pClipDescriptor->m_skeleton.GetResourceID() == m_workspaceResource->GetSkeleton()->GetResourceID();
                };

                auto results = m_pToolsContext->m_pResourceDatabase->GetAllResourcesOfTypeFiltered( AnimationClip::GetStaticResourceTypeID(), SkeletonFilter );
                auto pSkeleton = m_workspaceResource->GetSkeleton();

                m_clipsWithSameSkeleton.clear();
                for ( auto const& result : results )
                {
                    m_clipsWithSameSkeleton.emplace_back( result );
                }

                ApplyClipFilter();

                m_clipBrowserCacheRefreshTimer.Reset();
            }
        }

        // Filter clips
        //-------------------------------------------------------------------------

        if ( m_clipBrowserFilter.DrawAndUpdate() )
        {
            ApplyClipFilter();
        }

        // Draw Clips
        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            if ( ImGui::BeginTable( "ClipBrowser", 1, 0, ImVec2( -1, -1 ) ) )
            {
                ImGuiListClipper clipper;
                clipper.Begin( (int32_t) m_filteredClips.size() );
                while ( clipper.Step() )
                {
                    for ( int32_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        if ( ImGuiX::FlatButton( m_filteredClips[i].c_str() ) )
                        {
                            m_pToolsContext->TryOpenResource( m_filteredClips[i] );
                        }
                    }
                }

                ImGui::EndTable();
            }
        }
    }

    //-------------------------------------------------------------------------

    bool AnimationClipWorkspace::Save()
    {
        auto const status = m_eventEditor.GetTrackContainerValidationStatus();
        if ( status == Timeline::Track::Status::HasErrors )
        {
            pfd::message( "Invalid Events", "This animation clip has one or more invalid event tracks. These events will not be available in the game!", pfd::choice::ok, pfd::icon::error ).result();
        }

        if ( !TWorkspace<AnimationClip>::Save() )
        {
            return false;
        }

        m_propertyGrid.ClearDirty();
        return true;
    }

    void AnimationClipWorkspace::ReadCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& descriptorObjectValue )
    {
        m_eventEditor.Serialize( typeRegistry, descriptorObjectValue );
    }

    void AnimationClipWorkspace::WriteCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer )
    {
        reinterpret_cast<Timeline::TimelineEditor&>( m_eventEditor ).Serialize( typeRegistry, writer );
    }
}