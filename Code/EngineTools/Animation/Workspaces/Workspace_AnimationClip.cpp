#include "Workspace_AnimationClip.h"
#include "EngineTools/Animation/Events/AnimationEventEditor.h"
#include "EngineTools/Animation/Events/AnimationEventData.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Core/Widgets/InterfaceHelpers.h"
#include "Engine/Animation/Components/Component_AnimationClipPlayer.h"
#include "Engine/Animation/Systems/EntitySystem_Animation.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "System/Animation/AnimationPose.h"
#include "System/Math/MathStringHelpers.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_WORKSPACE_FACTORY( AnimationClipWorkspaceFactory, AnimationClip, AnimationClipWorkspace );

    //-------------------------------------------------------------------------

    AnimationClipWorkspace::AnimationClipWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : TResourceWorkspace<AnimationClip>( pToolsContext, pWorld, resourceID )
        , m_propertyGrid( m_pToolsContext )
        , m_eventEditor( *pToolsContext->m_pTypeRegistry )
    {
        auto OnBeginMod = [this]( Timeline::TrackContainer* pContainer )
        {
            if ( pContainer != m_eventEditor.GetTrackContainer() )
            {
                return;
            }

            BeginModification();
        };

        auto OnEndMod = [this]( Timeline::TrackContainer* pContainer )
        {
            if ( pContainer != m_eventEditor.GetTrackContainer() )
            {
                return;
            }

            EndModification();
        };

        m_eventEditor.SetLooping( true );

        m_beginModEventID = Timeline::TrackContainer::s_onBeginModification.Bind( OnBeginMod );
        m_endModEventID = Timeline::TrackContainer::s_onEndModification.Bind( OnEndMod );

        m_propertyGridPreEditEventBindingID = m_propertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& info ) { PreEdit( info ); } );
        m_propertyGridPostEditEventBindingID = m_propertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& info ) { PostEdit( info ); } );
    }

    AnimationClipWorkspace::~AnimationClipWorkspace()
    {
        Timeline::TrackContainer::s_onBeginModification.Unbind( m_beginModEventID );
        Timeline::TrackContainer::s_onEndModification.Unbind( m_endModEventID );

        m_propertyGrid.OnPreEdit().Unbind( m_propertyGridPreEditEventBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_propertyGridPostEditEventBindingID );
    }

    void AnimationClipWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID topDockID = 0, bottomDockID = 0, bottomLeftDockID = 0, bottomRightDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.33f, &bottomDockID, &topDockID );
        ImGui::DockBuilderSplitNode( bottomDockID, ImGuiDir_Right, 0.25f, &bottomRightDockID, &bottomLeftDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetViewportWindowID(), topDockID );
        ImGui::DockBuilderDockWindow( m_timelineWindowName.c_str(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( m_trackDataWindowName.c_str(), bottomRightDockID );
        ImGui::DockBuilderDockWindow( m_detailsWindowName.c_str(), bottomRightDockID );
        ImGui::DockBuilderDockWindow( m_descriptorWindowName.c_str(), bottomRightDockID );
    }

    //-------------------------------------------------------------------------

    void AnimationClipWorkspace::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TResourceWorkspace<AnimationClip>::Initialize( context );

        m_timelineWindowName.sprintf( "Timeline##%u", GetID() );
        m_detailsWindowName.sprintf( "Details##%u", GetID() );
        m_trackDataWindowName.sprintf( "Track Data##%u", GetID() );

        CreatePreviewEntity();
    }

    void AnimationClipWorkspace::Shutdown( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity != nullptr );

        m_pPreviewEntity = nullptr;
        m_pAnimationComponent = nullptr;
        m_pMeshComponent = nullptr;

        TResourceWorkspace<AnimationClip>::Shutdown( context );
    }

    void AnimationClipWorkspace::CreatePreviewEntity()
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        // Create animation component
        m_pAnimationComponent = EE::New<AnimationClipPlayerComponent>( StringID( "Animation Component" ) );
        m_pAnimationComponent->SetAnimation( m_pResource.GetResourceID() );

        // Create entity
        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        m_pPreviewEntity->CreateSystem<AnimationSystem>();
        m_pPreviewEntity->AddComponent( m_pAnimationComponent );

        // Create the mesh component
        CreatePreviewMeshComponent();

        //-------------------------------------------------------------------------

        AddEntityToWorld( m_pPreviewEntity );
    }

    void AnimationClipWorkspace::CreatePreviewMeshComponent()
    {
        if ( m_pPreviewEntity == nullptr )
        {
            return;
        }

        // Load resource descriptor for skeleton to get the preview mesh
        auto pAnimClipDescriptor = GetDescriptorAs<AnimationClipResourceDescriptor>();
        if ( pAnimClipDescriptor->m_pSkeleton.IsValid() )
        {
            FileSystem::Path const resourceDescPath = GetFileSystemPath( pAnimClipDescriptor->m_pSkeleton.GetResourcePath() );
            SkeletonResourceDescriptor resourceDesc;
            if ( Resource::ResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, resourceDescPath, resourceDesc ) && resourceDesc.m_previewMesh.IsValid() )
            {
                // Create a preview mesh component
                m_pMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
                m_pMeshComponent->SetSkeleton( pAnimClipDescriptor->m_pSkeleton.GetResourceID() );
                if ( resourceDesc.m_previewMesh.IsValid() )
                {
                    m_pMeshComponent->SetMesh( resourceDesc.m_previewMesh.GetResourceID() );
                }
            }
        }

        if ( m_pMeshComponent != nullptr )
        {
            m_pPreviewEntity->AddComponent( m_pMeshComponent );
        }
    }

    void AnimationClipWorkspace::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        TResourceWorkspace<AnimationClip>::BeginHotReload( usersToBeReloaded, resourcesToBeReloaded );

        // If someone messed with this resource outside of this editor
        if ( m_pDescriptor == nullptr )
        {
            m_propertyGrid.SetTypeToEdit( nullptr );
        }

        if ( IsHotReloading() )
        {
            if ( m_pMeshComponent != nullptr )
            {
                m_pPreviewEntity->DestroyComponent( m_pMeshComponent );
                m_pMeshComponent = nullptr;
            }
        }
    }

    void AnimationClipWorkspace::EndHotReload()
    {
        TResourceWorkspace<AnimationClip>::EndHotReload();

        if ( m_pMeshComponent == nullptr )
        {
            CreatePreviewMeshComponent();
        }
    }

    //-------------------------------------------------------------------------

    void AnimationClipWorkspace::UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        if ( IsResourceLoaded() )
        {
            m_eventEditor.SetAnimationLengthAndFPS( m_pResource->GetNumFrames(), m_pResource->GetFPS() );

            // Update position
            //-------------------------------------------------------------------------

            if ( m_isRootMotionEnabled )
            {
                m_characterTransform = m_pResource->GetRootTransform( m_pAnimationComponent->GetAnimTime() );
            }
            else
            {
                m_characterTransform = Transform::Identity;
            }

            // HACK
            if ( m_pMeshComponent != nullptr )
            {
                m_pMeshComponent->SetWorldTransform( m_characterTransform );
            }

            // Draw in viewport
            //-------------------------------------------------------------------------

            auto drawingCtx = GetDrawingContext();
            m_pResource->GetRootMotion().DrawDebug( drawingCtx, Transform::Identity );

            if ( m_isPoseDrawingEnabled )
            {
                Pose const* pPose = m_pAnimationComponent->GetPose();
                if ( pPose != nullptr )
                {
                    drawingCtx.Draw( *pPose, m_characterTransform );
                }
            }
        }
        //-------------------------------------------------------------------------

        bool const isDescriptorWindowFocused = DrawDescriptorEditorWindow( context, pWindowClass );

        ImGui::SetNextWindowClass( pWindowClass );
        DrawTrackDataWindow( context );

        ImGui::SetNextWindowClass( pWindowClass );
        DrawTimelineWindow( context );

        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_detailsWindowName.c_str() ) )
        {
            m_propertyGrid.DrawGrid();
        }

        bool const isDetailsWindowFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_RootAndChildWindows );
        ImGui::End();

        //-------------------------------------------------------------------------

        // Enable the global timeline keyboard shortcuts
        if ( isFocused && !isDescriptorWindowFocused && !isDetailsWindowFocused )
        {
            m_eventEditor.HandleGlobalKeyboardInputs();
        }
    }

    void AnimationClipWorkspace::DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ImGui::NewLine();
        ImGui::Indent();

        auto PrintAnimDetails = [this] ( Color color )
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold, color );
            ImGui::Text( "Avg Linear Velocity: %.2f m/s", m_pResource->GetAverageLinearVelocity() );
            ImGui::Text( "Avg Angular Velocity: %.2f r/s", m_pResource->GetAverageAngularVelocity().ToFloat() );
            ImGui::Text( "Distance Covered: %.2fm", m_pResource->GetTotalRootMotionDelta().GetTranslation().GetLength3() );
            ImGui::Text( "Frame: %.f/%d", m_eventEditor.GetPlayheadPositionAsPercentage().ToFloat() * m_pResource->GetNumFrames(), m_pResource->GetNumFrames() );
            ImGui::Text( "Time: %.2fs/%0.2fs", m_eventEditor.GetPlayheadPositionAsPercentage().ToFloat() * m_pResource->GetDuration(), m_pResource->GetDuration().ToFloat() );
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

    void AnimationClipWorkspace::DrawWorkspaceToolbarItems( UpdateContext const& context )
    {
        if ( ImGui::BeginMenu( EE_ICON_COG" Debug Options" ) )
        {
            ImGui::Checkbox( "Root Motion Enabled", &m_isRootMotionEnabled );
            ImGui::Checkbox( "Draw Bone Pose", &m_isPoseDrawingEnabled );

            ImGui::EndMenu();
        }
    }

    void AnimationClipWorkspace::DrawTimelineWindow( UpdateContext const& context )
    {
        // Draw timeline window
        //-------------------------------------------------------------------------

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
        if ( ImGui::Begin( m_timelineWindowName.c_str() ) )
        {
            if ( IsWaitingForResource() )
            {
                ImGui::Text( "Loading:" );
                ImGui::SameLine();
                ImGuiX::DrawSpinner( "Loading" );
            }
            else if ( HasLoadingFailed() )
            {
                ImGui::Text( "Loading Failed: %s", m_pResource.GetResourceID().c_str() );
            }
            else
            {
                // Track editor and property grid
                //-------------------------------------------------------------------------

                m_eventEditor.UpdateAndDraw( context, m_pAnimationComponent );

                // Transfer dirty state from property grid
                if ( m_propertyGrid.IsDirty() )
                {
                    m_eventEditor.MarkDirty();
                }

                // Handle selection changes
                auto const& selectedItems = m_eventEditor.GetSelectedItems();
                if ( !selectedItems.empty() )
                {
                    auto pAnimEventItem = static_cast<EventItem*>( selectedItems.back() );
                    m_propertyGrid.SetTypeToEdit( pAnimEventItem->GetEvent() );
                }
                else // Clear property grid
                {
                    m_propertyGrid.SetTypeToEdit( nullptr );
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void AnimationClipWorkspace::DrawTrackDataWindow( UpdateContext const& context )
    {
        if ( ImGui::Begin( m_trackDataWindowName.c_str() ) )
        {
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
                                Transform const& boneTransform = ( boneIdx == 0 ) ? m_pResource->GetRootTransform( m_pAnimationComponent->GetAnimTime() ) : pPose->GetGlobalTransform( boneIdx );

                                ImGui::TableNextColumn();
                                ImGui::Text( "%d. %s", boneIdx, pSkeleton->GetBoneID( boneIdx ).c_str() );

                                ImGui::TableNextColumn();
                                ImGuiX::DisplayTransform( boneTransform );
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
        ImGui::End();
    }

    //-------------------------------------------------------------------------

    bool AnimationClipWorkspace::IsDirty() const
    {
        if ( TResourceWorkspace<AnimationClip>::IsDirty() )
        {
            return true;
        }

        return m_eventEditor.IsDirty();
    }

    bool AnimationClipWorkspace::Save()
    {
        if ( !TResourceWorkspace<AnimationClip>::Save() )
        {
            return false;
        }

        m_propertyGrid.ClearDirty();
        return true;
    }

    void AnimationClipWorkspace::SerializeCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& descriptorObjectValue )
    {
        m_eventEditor.Serialize( typeRegistry, descriptorObjectValue );
    }

    void AnimationClipWorkspace::SerializeCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer )
    {
        static_cast<Timeline::TimelineEditor&>( m_eventEditor ).Serialize( typeRegistry, writer );
    }
}