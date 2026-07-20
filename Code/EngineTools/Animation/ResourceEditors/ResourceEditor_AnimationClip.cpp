#include "ResourceEditor_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Timeline/TimelineEditor.h"
#include "EngineTools/Core/SystemDialogs.h"
#include "Engine/Animation/Components/Component_AnimationClipPlayer.h"
#include "Engine/Animation/Systems/EntitySystem_Animation.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "Engine/Animation/AnimationPose.h"
#include "Base/Math/MathUtils.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_EDITOR_FACTORY( AnimationClipEditorFactory, AnimationClip, AnimationClipEditor );

    //-------------------------------------------------------------------------

    AnimationClipEditor::AnimationClipEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld )
        : TResourceEditor<AnimationClip>( pToolsContext, resourceID, pWorld )
        , m_propertyGrid( m_pToolsContext )
        , m_animationClipBrowser( pToolsContext )
    {
        //-------------------------------------------------------------------------

        auto const PreDescEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction == nullptr );
            EE_ASSERT( IsDataFileLoaded() );
            BeginDataFileModification();
        };

        auto const PostDescEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );
            EE_ASSERT( IsDataFileLoaded() );
            EndDataFileModification();
        };

        m_propertyGridPreEditEventBindingID = m_propertyGrid.OnPreEdit().Bind( PreDescEdit );
        m_propertyGridPostEditEventBindingID = m_propertyGrid.OnPostEdit().Bind( PostDescEdit );
    }

    AnimationClipEditor::~AnimationClipEditor()
    {
        m_propertyGrid.OnPreEdit().Unbind( m_propertyGridPreEditEventBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_propertyGridPostEditEventBindingID );
    }

    void AnimationClipEditor::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        // Split
        //-------------------------------------------------------------------------

        ImGuiID leftDockID0 = 0;
        ImGuiID leftDockID1 = 0;
        ImGuiID centerDockID0 = 0;
        ImGuiID centerDockID1 = 0;
        ImGuiID rightDockID0 = 0;
        ImGuiID rightDockID1 = 0;
        ImGuiID rightDockID2 = 0;

        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.25f, &leftDockID0, &centerDockID0 );
        ImGui::DockBuilderSplitNode( centerDockID0, ImGuiDir_Right, 0.33f, &rightDockID0, &centerDockID0 );

        ImGui::DockBuilderSplitNode( leftDockID0, ImGuiDir_Up, 0.7f, &leftDockID0, &leftDockID1 );
        ImGui::DockBuilderSplitNode( centerDockID0, ImGuiDir_Down, 0.5f, &centerDockID0, &centerDockID1 );

        ImGui::DockBuilderSplitNode( rightDockID0, ImGuiDir_Up, 0.7f, &rightDockID0, &rightDockID2 );
        ImGui::DockBuilderSplitNode( rightDockID0, ImGuiDir_Up, 0.7f, &rightDockID0, &rightDockID1 );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Skeleton Outline" ).c_str(), leftDockID0 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Float Curves" ).c_str(), leftDockID1 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Bone Info" ).c_str(), leftDockID1 );
        
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Timeline" ).c_str(), centerDockID0 );

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Clip Browser" ).c_str(), rightDockID0 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_dataFileWindowName ).c_str(), rightDockID1 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Event Details" ).c_str(), rightDockID2 );
    }

    //-------------------------------------------------------------------------

    void AnimationClipEditor::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TResourceEditor<AnimationClip>::Initialize( context );

        m_characterPoseUpdateRequested = true;

        //-------------------------------------------------------------------------

        CreateToolWindow( "Timeline", [this] ( UpdateContext const& context, bool isFocused ) { DrawTimelineWindow( context, isFocused ); }, ImVec2( 0, 0 ), true );
        CreateToolWindow( "Event Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawDetailsWindow( context, isFocused ); } );
        CreateToolWindow( "Clip Browser", [this] ( UpdateContext const& context, bool isFocused ) { DrawClipBrowser( context, isFocused ); } );
        CreateToolWindow( "Skeleton Outline", [this] ( UpdateContext const& context, bool isFocused ) { DrawSkeletonOutlineWindow( context, isFocused ); } );
        CreateToolWindow( "Bone Info", [this] ( UpdateContext const& context, bool isFocused ) { DrawBoneInfoWindow( context, isFocused ); } );
        CreateToolWindow( "Float Curves", [this] ( UpdateContext const& context, bool isFocused ) { DrawFloatCurvesWindow( context, isFocused ); } );
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

        // Create the p[review mesh components
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
                    pSecondaryMeshComponent->SetAttachmentSocketID( pPrimarySkeleton->GetPreviewAttachmentBoneIDForSecondarySkeleton( pSecondarySkeleton ) );
                    m_pPreviewEntity->AddComponent( pSecondaryMeshComponent, m_pMeshComponent->GetID() );
                }
            }
        }

        // Set up skeleton attachments
        m_secondarySkeletonAttachmentSocketIDs.clear();
        for ( auto pSecondaryAnimation : m_editedResource->GetSecondaryAnimations() )
        {
            Skeleton const* pSecondarySkeleton = pSecondaryAnimation->GetSkeleton();
            m_secondarySkeletonAttachmentSocketIDs.emplace_back( pPrimarySkeleton->GetPreviewAttachmentBoneIDForSecondarySkeleton( pSecondarySkeleton ) );
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

    void AnimationClipEditor::OnDataFileUnload()
    {
        EE::Delete( m_pTimelineEditor );
        m_propertyGrid.SetTypeToEdit( nullptr );
        m_animationClipBrowser.SetSkeleton( ResourceID() );
    }

    void AnimationClipEditor::OnDataFileLoadCompleted()
    {
        m_characterPoseUpdateRequested = true;

        if ( IsDataFileLoaded() )
        {
            m_animationClipBrowser.SetSkeleton( GetDataFile<AnimationClipResourceDescriptor>()->m_skeleton.GetResourceID() );

            AnimationClipResourceDescriptor* pDescriptor = GetDataFile<AnimationClipResourceDescriptor>();
            pDescriptor->m_events.FillAllowedTrackTypes( *m_pToolsContext->m_pTypeRegistry );
            m_pTimelineEditor = EE::New<Timeline::TimelineEditor>( &pDescriptor->m_events, [this] () { BeginDataFileModification(); }, [this] () { EndDataFileModification(); } );
            m_pTimelineEditor->SetLooping( true );
        }
    }

    void AnimationClipEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource && m_editedResource.IsLoaded() )
        {
            EE_ASSERT( m_pTimelineEditor != nullptr );

            m_pTimelineEditor->SetLength( (float) m_editedResource->GetNumFrames() - 1 );
            m_pTimelineEditor->SetTimeUnitConversionFactor( m_editedResource->IsSingleFrameAnimation() ? 30 : m_editedResource->GetFPS() );
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

    void AnimationClipEditor::WorldUpdate( EntityWorldUpdateContext const& updateContext )
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

            auto drawingCtx = GetDebugDrawContext();
            m_editedResource->GetRootMotion().DrawDebug( drawingCtx, Transform::Identity );

            if ( m_isPoseDrawingEnabled && m_pAnimationComponent != nullptr )
            {
                DrawOptions options;
                options.m_drawBoneNames = m_shouldDrawBoneNames;

                // Draw the main pose
                Pose const* pPose = m_pAnimationComponent->GetPrimaryPose();
                if ( pPose != nullptr )
                {
                    if ( !m_isolateSelectedBones || m_selectedBoneIDs.empty() )
                    {
                        pPose->DrawDebug( drawingCtx, m_characterTransform, m_skeletonLOD, options );
                    }
                    else
                    {
                        TVector<int32_t> boneFilter;
                        for ( StringID selectedBoneID : m_selectedBoneIDs )
                        {
                            boneFilter.emplace_back( pPose->GetSkeleton()->GetBoneIndex( selectedBoneID ) );
                        }

                        pPose->DrawDebug( drawingCtx, m_characterTransform, m_skeletonLOD, options );
                    }
                }

                // Draw the secondary animation poses
                int32_t const numSecondaryPoses = m_pAnimationComponent->GetNumSecondaryPoses();
                for ( int32_t i = 0; i < numSecondaryPoses; i++ )
                {
                    Pose const* pSecondaryPose = m_pAnimationComponent->GetSecondaryPoses()[i];

                    Transform secondaryPoseTransform = m_characterTransform;
                    if ( !m_secondarySkeletonAttachmentSocketIDs.empty() )
                    {
                        // If we have a mesh component use that for the attachment transform
                        if ( m_pMeshComponent != nullptr )
                        {
                            m_pMeshComponent->GetAttachmentSocketTransform( m_secondarySkeletonAttachmentSocketIDs[i] );
                        }
                        else // Try use the bone transform from the primary pose
                        {
                            int32_t const boneIdx = pPose->GetSkeleton()->GetBoneIndex( m_secondarySkeletonAttachmentSocketIDs[i] );
                            if ( boneIdx != InvalidIndex )
                            {
                                secondaryPoseTransform = pPose->GetModelSpaceTransform( boneIdx );
                            }
                        }
                    }

                    options.m_boneColor = Colors::Cyan;
                    pSecondaryPose->DrawDebug( drawingCtx, secondaryPoseTransform, m_skeletonLOD, options );
                }
            }

            // Draw capsule
            //-------------------------------------------------------------------------

            if ( m_isPreviewCapsuleDrawingEnabled )
            {
                Transform capsuleTransform = m_characterTransform;
                capsuleTransform.AddTranslation( capsuleTransform.GetAxisZ() * ( m_previewCapsuleHalfHeight + m_previewCapsuleRadius ) );
                drawingCtx.DrawWireCapsule( capsuleTransform, m_previewCapsuleRadius, m_previewCapsuleHalfHeight, Colors::LimeGreen, 3.0f );
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

            EE_ASSERT( m_pTimelineEditor != nullptr );

            Percentage const percentageThroughAnimation = m_pTimelineEditor->GetPlayheadTimeAsPercentage();
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

    void AnimationClipEditor::DrawHelpMenu() const
    {
        DrawHelpTextRow( "Navigate Timeline", "Left/Right arrows" );
        DrawHelpTextRow( "Create Event", "Enter (with track selected)" );
    }

    void AnimationClipEditor::DrawMenu( UpdateContext const& context )
    {
        TResourceEditor<AnimationClip>::DrawMenu( context );

        if ( ImGui::BeginMenu( EE_ICON_TUNE" Options" ) )
        {
            ImGui::Checkbox( "Enable Root Motion", &m_isRootMotionEnabled );

            ImGui::EndMenu();
        }
    }

    void AnimationClipEditor::ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        // Clip Info
        //-------------------------------------------------------------------------

        auto PrintAnimDetails = [this] ( Color color )
        {
            Percentage const currentTime = m_pTimelineEditor->GetPlayheadTimeAsPercentage();
            uint32_t const numFrames = m_editedResource->GetNumFrames();
            FrameTime const frameTime = m_editedResource->GetFrameTime( currentTime );

            ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold, color );
            ImGui::Text( "Avg Linear Velocity: %.2f m/s", m_editedResource->GetAverageLinearVelocity() );
            ImGui::Text( "Avg Angular Velocity: %.2f r/s", m_editedResource->GetAverageAngularVelocity().ToFloat() );
            ImGui::Text( "Distance Covered: %.2fm", m_editedResource->GetTotalRootMotionDelta().GetTranslation().GetLength3() );

            ImGui::Text( "Num Bones: %d", m_editedResource->GetSkeleton()->GetNumBones() );

            ImGui::NewLine();

            ImGui::Text( "Frame: %.2f/%d", frameTime.ToFloat(), numFrames - 1 );
            ImGui::Text( "Time: %.2fs/%0.2fs", m_pTimelineEditor->GetPlayheadTimeAsPercentage().ToFloat() * m_editedResource->GetDuration(), m_editedResource->GetDuration().ToFloat() );
            ImGui::Text( "Percentage: %.2f%%", m_pTimelineEditor->GetPlayheadTimeAsPercentage().ToFloat() * 100 );

            if ( m_editedResource->IsAdditive() )
            {
                ImGui::Text( "Additive" );
            }
        };

        ImGui::NewLine();

        ImGuiX::DrawShadowedText( Colors::Yellow, PrintAnimDetails );
    }

    bool AnimationClipEditor::ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport )
    {
        if ( ImGui::Checkbox( "Show Floor", &m_showFloor ) )
        {
            SetFloorVisibility( m_showFloor );
        }

        ImGui::BeginDisabled( m_pMeshComponent == nullptr );
        if ( ImGui::Checkbox( "Show Mesh", &m_showMesh ) )
        {
            m_pMeshComponent->SetVisible( m_showMesh );
        }
        ImGui::EndDisabled();

        ImGui::Checkbox( "Draw Bone Pose", &m_isPoseDrawingEnabled );

        ImGui::Checkbox( "Draw Bone Names", &m_shouldDrawBoneNames );

        ImGuiX::Checkbox( "Isolate Selected Bones", &m_isolateSelectedBones );

        ImGui::SeparatorText( "Capsule Debug" );

        ImGui::Checkbox( "Show Preview Capsule", &m_isPreviewCapsuleDrawingEnabled );

        constexpr static char const * const pHalfHeightStr = "Half-Height";
        constexpr static char const * const pRadiusStr = "Radius";
        float const offset = ImGui::GetCursorPos().x + Math::Max( ImGui::CalcTextSize( pHalfHeightStr ).x, ImGui::CalcTextSize( pRadiusStr ).x ) + ImGui::GetStyle().ItemSpacing.x;

        ImGui::AlignTextToFramePadding();
        ImGui::Text( pHalfHeightStr );
        ImGui::SameLine( offset );
        if ( ImGui::InputFloat( "##HH", &m_previewCapsuleHalfHeight, 0.05f ) )
        {
            m_previewCapsuleHalfHeight = Math::Clamp( m_previewCapsuleHalfHeight, 0.05f, 10.0f );
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text( pRadiusStr );
        ImGui::SameLine( offset );
        if ( ImGui::InputFloat( "##R", &m_previewCapsuleRadius, 0.01f ) )
        {
            m_previewCapsuleRadius = Math::Clamp( m_previewCapsuleRadius, 0.01f, 5.0f );
        }

        return true;
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
            EE_ASSERT( m_pTimelineEditor != nullptr );

            // Track editor and property grid
            //-------------------------------------------------------------------------

            m_pTimelineEditor->UpdateAndDraw( GetEntityWorld()->GetTimeScale() * context.GetDeltaTime() );

            // Handle selection changes
            auto const& selectedItems = m_pTimelineEditor->GetSelectedItems();
            if ( !selectedItems.empty() )
            {
                auto pEventItem = Cast<EventTrackItem>( selectedItems.back() );
                if ( pEventItem->GetEvent() != m_propertyGrid.GetEditedType() )
                {
                    m_propertyGrid.SetTypeToEdit( pEventItem->GetEvent() );
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

    void AnimationClipEditor::DrawSkeletonOutlineWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsResourceLoaded() )
        {
            ImGui::Text( "Nothing to show!" );
            return;
        }

        //-------------------------------------------------------------------------

        bool isUsingLowLOD = m_skeletonLOD == Skeleton::LOD::Low;
        if ( ImGui::Checkbox( "Use Low Skeleton LOD", &isUsingLowLOD ) )
        {
            m_skeletonLOD = isUsingLowLOD ? Skeleton::LOD::Low : Skeleton::LOD::High;
        }

        float const openSkeletonButtonWidth = ImGuiX::CalculateButtonWidth( EE_ICON_SKULL" Open Skeleton" );
        ImGui::SameLine( ImGui::GetContentRegionAvail().x - openSkeletonButtonWidth + ImGui::GetStyle().ItemSpacing.x, 0 );

        if ( ImGui::Button( EE_ICON_SKULL" Open Skeleton", ImVec2( openSkeletonButtonWidth, 0 ) ) )
        {
            Skeleton const* pSkeleton = m_editedResource.GetPtr()->GetSkeleton();
            m_pToolsContext->TryOpenResource( pSkeleton->GetResourceID() );
        }

        // Draw Tree
        //-------------------------------------------------------------------------

        AnimationClip const* pAnimation = m_editedResource.GetPtr();
        Skeleton const* pSkeleton = pAnimation->GetSkeleton();
        int32_t const numBones = pAnimation->GetNumBones();

        //-------------------------------------------------------------------------

        auto ApplySelectionRequests = [this, pSkeleton, numBones] ( ImGuiMultiSelectIO* pMSIO )
        {
            for ( ImGuiSelectionRequest const& req : pMSIO->Requests )
            {
                if ( req.Type == ImGuiSelectionRequestType_SetAll )
                {
                    m_selectedBoneIDs.clear();

                    if ( req.Selected )
                    {
                        for ( int32_t i = 0; i < numBones; i++ )
                        {
                            m_selectedBoneIDs.emplace_back( pSkeleton->GetBoneID( i ) );
                        }
                    }
                }
                else if ( req.Type == ImGuiSelectionRequestType_SetRange )
                {
                    if ( req.Selected )
                    {
                        for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                        {
                            StringID const boneID = pSkeleton->GetBoneID( (int32_t) i );
                            VectorEmplaceBackUnique( m_selectedBoneIDs, boneID );
                        }
                    }
                    else
                    {
                        for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                        {
                            StringID const boneID = pSkeleton->GetBoneID( (int32_t) i );
                            m_selectedBoneIDs.erase_first_unsorted( boneID );
                        }
                    }
                }
            }
        };

        ImGuiMultiSelectFlags const selectionFlags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_ClearOnClickVoid;
        ImGuiMultiSelectIO* pMSIO = ImGui::BeginMultiSelect( selectionFlags, -1, pAnimation->GetNumBones() );
        ApplySelectionRequests( pMSIO );

        static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
        if ( ImGui::BeginTable( "BoneTreeTable", 2, flags ) )
        {
            // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
            ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "LOD", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40.0f );

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            DrawSkeletonTreeRow( m_pSkeletonTreeRoot );

            ImGui::EndTable();
        }

        pMSIO = ImGui::EndMultiSelect();
        ApplySelectionRequests( pMSIO );
    }

    void AnimationClipEditor::DrawDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        m_propertyGrid.UpdateAndDraw();
    }

    void AnimationClipEditor::DrawClipBrowser( UpdateContext const& context, bool isFocused )
    {
        m_animationClipBrowser.Draw( context, isFocused );
    }

    void AnimationClipEditor::DrawFloatCurvesWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        //AnimationClip const* pAnimation = m_editedResource.GetPtr();
        //int32_t const numFloatCurves = pAnimation->GetNumFloatCurves();

        //if ( numFloatCurves == 0 )
        //{
        //    ImGui::Text( "No Float Curves" );
        //}

        ////-------------------------------------------------------------------------

        //if ( ImGui::BeginTable( "FloatCurveTable", 2, ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
        //{
        //    ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
        //    ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 100.0f );
        //    ImGui::TableSetupScrollFreeze( 0, 1 );

        //    ImGui::TableHeadersRow();

        //    //-------------------------------------------------------------------------

        //    TInlineVector<float, 300> floatCurveValues;
        //    pAnimation->GetFloatCurveValues( m_currentAnimTime, floatCurveValues );

        //    //-------------------------------------------------------------------------

        //    for ( int32_t i = 0; i < numFloatCurves; i++ )
        //    {
        //        StringID const curveID = pAnimation->GetFloatCurveID( i );

        //        ImGui::TableNextRow();

        //        ImGui::TableNextColumn();
        //        ImGui::Text( curveID.IsValid() ? curveID.c_str() : "Invalid ID");

        //        ImGui::TableNextColumn();
        //        ImGui::TextColored( Color::EvaluateRedGreenGradient( floatCurveValues[i] ).ToFloat4(), "%.5f", floatCurveValues[i] );
        //    }

        //    ImGui::EndTable();
        //}
    }

    void AnimationClipEditor::DrawBoneInfoWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        AnimationClip const* pAnimation = m_editedResource.GetPtr();
        Skeleton const* pSkeleton = pAnimation->GetSkeleton();
        Pose const* pPose = m_pAnimationComponent->GetPrimaryPose();

        if ( ImGui::BeginTable( "BoneTreeTable", 3, ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
        {
            ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Parent Space", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Model Space", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupScrollFreeze( 0, 1 );

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            for ( StringID const& boneID : m_selectedBoneIDs )
            {
                int32_t const selectedBoneIdx = pSkeleton->GetBoneIndex( boneID );
                EE_ASSERT( selectedBoneIdx != InvalidIndex );
                Transform const parentSpaceBoneTransform = ( selectedBoneIdx == 0 ) ? m_editedResource->GetRootTransform( m_pAnimationComponent->GetAnimTime() ) : pPose->GetTransform( selectedBoneIdx );
                Transform const modelSpaceBoneTransform = pPose->GetModelSpaceTransform( selectedBoneIdx );

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text( "%d. %s", selectedBoneIdx, pSkeleton->GetBoneID( selectedBoneIdx ).c_str() );

                ImGui::TableNextColumn();
                ImGuiX::DrawTransform( parentSpaceBoneTransform );

                ImGui::TableNextColumn();
                ImGuiX::DrawTransform( modelSpaceBoneTransform );
            }

            //-------------------------------------------------------------------------

            ImGui::EndTable();
        }
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

        //-------------------------------------------------------------------------

        ImGui::TableNextRow();

        //-------------------------------------------------------------------------
        // Draw Label
        //-------------------------------------------------------------------------

        bool const isHighLOD = pSkeleton->IsBoneHighLOD( boneIdx );
        InlineString const boneLabel( InlineString::CtorSprintf(), "%d. %s", boneIdx, currentBoneID.c_str() );

        ImGui::TableNextColumn();

        int32_t treeNodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow;
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

        ImGui::SetNextItemSelectionUserData( boneIdx );
        {
            ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::MediumBold : ImGuiX::Font::Default, ImGuiX::Style::s_colorText );
            pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
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
        // Draw LOD
        //-------------------------------------------------------------------------

        ImGui::TableNextColumn();

        if ( isHighLOD )
        {
            ImGui::TextColored( Colors::Lime.ToFloat4(), "High" );
        }
        else
        {
            ImGui::TextColored( Colors::Orange.ToFloat4(), "Low" );
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
        Timeline::Track::Status const status = m_pTimelineEditor->GetTrackContainerValidationStatus();
        if ( status == Timeline::Track::Status::HasErrors )
        {
            MessageDialog::Error( "Invalid Events", "This animation clip has one or more invalid event tracks. These events will not be available in the game!" );
        }

        if ( !TResourceEditor<AnimationClip>::SaveData() )
        {
            return false;
        }

        m_propertyGrid.ClearDirty();

        return true;
    }

    void AnimationClipEditor::PreUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        TResourceEditor<AnimationClip>::PreUndoRedo( operation, pAction );
        m_propertyGrid.SetTypeToEdit( nullptr );
    }
}