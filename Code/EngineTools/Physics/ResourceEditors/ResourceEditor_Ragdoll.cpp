#include "ResourceEditor_Ragdoll.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/PropertyGrid/PropertyGridTypeEditingRules.h"
#include "EngineTools/Core/SystemDialogs.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Animation/AnimationPose.h"
#include "Base/Math/Intersection.h"
#include "../../Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    EE_RESOURCE_EDITOR_FACTORY( RagdollEditorFactory, RagdollDefinition, RagdollEditor );

    //-------------------------------------------------------------------------

    RagdollEditor::RagdollEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld )
        : TResourceEditor<RagdollDefinition>( pToolsContext, resourceID, pWorld )
        , m_propertyGrid( pToolsContext )
        , m_animPicker( *pToolsContext )
    {
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowNonUniformScale, true );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowCoordinateSpaceSwitching, true );
        m_gizmo.SetMode( ImGuiX::Gizmo::Mode::Translation );

        auto OnPreEdit = [this] ( PropertyEditInfo const& info )
        {
            BeginDataFileModification();
        };

        auto OnPostEdit = [this] ( PropertyEditInfo const& info )
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );
            EndDataFileModification();
        };

        m_preEditEventBindingID = m_propertyGrid.OnPreEdit().Bind( OnPreEdit );
        m_postEditEventBindingID = m_propertyGrid.OnPostEdit().Bind( OnPostEdit );

        //-------------------------------------------------------------------------

        auto CustomResourceFilter = [this] ( Resource::ResourceDescriptor const* pDesc )
        {
            auto pClipDesc = Cast<Animation::AnimationClipResourceDescriptor>( pDesc );
            return pClipDesc->m_skeleton == m_skeleton;
        };

        m_animPicker.SetRequiredResourceType( Animation::AnimationClip::GetStaticResourceTypeID() );
        m_animPicker.SetCustomResourceFilter( CustomResourceFilter );
    }

    RagdollEditor::~RagdollEditor()
    {
        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
        EE_ASSERT( !m_skeleton.IsSet() || m_skeleton.IsUnloaded() );

        m_propertyGrid.OnPreEdit().Unbind( m_preEditEventBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_postEditEventBindingID );
    }

    //-------------------------------------------------------------------------
    // Editor Lifetime
    //-------------------------------------------------------------------------

    void RagdollEditor::Initialize( UpdateContext const& context )
    {
        TResourceEditor::Initialize( context );

        //-------------------------------------------------------------------------

        CreateToolWindow( "Outliner", [this] ( UpdateContext const& context, bool isFocused ) { DrawOutlinerWindow( context, isFocused ); } );
        CreateToolWindow( "Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawDetailsWindow( context, isFocused ); } );
        CreateToolWindow( "Preview Controls", [this] ( UpdateContext const& context, bool isFocused ) { DrawPreviewControlsWindow( context, isFocused ); } );
        CreateToolWindow( "Self Collision", [this] ( UpdateContext const& context, bool isFocused ) { DrawSelfCollisionEditorWindow( context, isFocused ); } );
    }

    void RagdollEditor::Shutdown( UpdateContext const& context )
    {
        // Unload preview animation
        if ( m_previewAnimation.IsSet() && !m_previewAnimation.IsUnloaded() )
        {
            UnloadResource( &m_previewAnimation );
        }

        //-------------------------------------------------------------------------

        TResourceEditor<RagdollDefinition>::Shutdown( context );
    }

    void RagdollEditor::OnDataFileUnload()
    {
        UnloadSkeleton();
    }

    void RagdollEditor::OnDataFileLoadCompleted()
    {
        if ( IsDataFileLoaded() )
        {
            LoadSkeleton();
        }
    }

    void RagdollEditor::OnResourceUnload( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_skeleton )
        {
            DestroyPreviewEntity();
            DestroySkeletonTree();
        }
    }

    void RagdollEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_skeleton && m_skeleton.IsLoaded() )
        {
            FixUpDescriptorBodiesAndSettings();
            CreatePreviewEntity();
            CreateSkeletonTree();
        }
    }

    void RagdollEditor::LoadSkeleton()
    {
        EE_ASSERT( !m_skeleton.IsSet() || !m_skeleton.WasRequested() );

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        m_skeleton = pRagdollDescriptor->m_skeleton;
        if ( m_skeleton.IsSet() )
        {
            LoadResource( &m_skeleton );
        }
        else
        {
            MessageDialog::Error( "Invalid Descriptor", "Descriptor doesnt have a skeleton set!\r\nPlease set a valid skeleton via the descriptor editor" );
        }
    }

    void RagdollEditor::UnloadSkeleton()
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

    //-------------------------------------------------------------------------
    // Descriptor
    //-------------------------------------------------------------------------

    void RagdollEditor::FixUpDescriptorBodiesAndSettings()
    {
        EE_ASSERT( IsDataFileLoaded() && m_skeleton.IsLoaded() );
        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        pRagdollDescriptor->CleanupInvalidSettings( m_skeleton.GetPtr() );
    }

    //-------------------------------------------------------------------------
    // Update
    //-------------------------------------------------------------------------

    void RagdollEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        // Execute any schedule commands
        if ( m_commandStack.HasCommands() )
        {
            m_commandStack.ExecuteCommands();
            m_propertyGrid.SetTypeToEdit( nullptr );
        }

        // Ensure selection is valid
        IReflectedType* pTypeToEdit = GetTypeToEditBasedOnSelection();
        if ( pTypeToEdit == nullptr )
        {
            m_selectedElement.Clear();
        }

        //-------------------------------------------------------------------------

        if ( IsPreviewing() )
        {
            // Calculate the desired input pose
            Seconds const animDeltaTime = m_pWorld->IsPaused() ? Seconds( 0.0f ) : ( context.GetDeltaTime() * m_pWorld->GetTimeScale() );
            CalculateInputPose( animDeltaTime );

            // Draw the input pose
            auto drawingCtx = GetDebugDrawContext();
            if ( m_drawInputAnimationPose )
            {
                Animation::DrawOptions options;
                options.m_lineThickness = IsPreviewing() ? 5.0f : 2.0f;

                m_pInputPose->DrawDebug( drawingCtx, m_previewStartTransform, Animation::Skeleton::LOD::High, options );
            }
        }
    }

    void RagdollEditor::WorldUpdate( EntityWorldUpdateContext const& updateContext )
    {
        if ( !IsPreviewing() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        // Update ragdoll
        if ( updateContext.GetUpdateStage() == UpdateStage::PrePhysics )
        {
            if ( !updateContext.IsWorldPaused() )
            {
                Transform const worldTransform = m_pMeshComponent->GetWorldTransform();
                m_pRagdoll->SetPoseFollowing( m_enablePoseFollowing );
                m_pRagdoll->Update( updateContext.GetDeltaTime(), worldTransform, m_pInputPose );
            }
        }
        else if ( updateContext.GetUpdateStage() == UpdateStage::PostPhysics )
        {
            auto drawingCtx = GetDebugDrawContext();

            // Copy the animation pose
            m_pFinalPose->CopyFrom( m_pInputPose );

            // Retrieve the ragdoll pose
            Transform const& worldTransform = m_pMeshComponent->GetWorldTransform();
            bool const shouldStopPreview = !m_pRagdoll->GetPose( worldTransform, m_pInputPose );

            // Apply physics blend weight
            Animation::Blender::ParentSpaceBlend( Animation::Skeleton::LOD::High, m_pFinalPose, m_pInputPose, m_physicsBlendWeight, nullptr, m_pFinalPose );
            m_pFinalPose->CalculateModelSpaceTransforms();

            // Draw final animation pose
            if ( m_drawFinalAnimationPose )
            {
                drawingCtx.Draw( *m_pFinalPose, worldTransform );
            }

            // Draw ragdoll pose
            if ( m_drawRagdoll )
            {
                drawingCtx.Draw( *m_pRagdoll );
            }

            // Update mesh pose
            if ( m_pMeshComponent->IsInitialized() && m_pMeshComponent->HasMeshResourceSet() )
            {
                m_pMeshComponent->SetPose( m_pFinalPose );
                m_pMeshComponent->FinalizePose();
            }

            //-------------------------------------------------------------------------

            if ( shouldStopPreview )
            {
                EE_LOG_WARNING( LogCategory::Physics, "Ragdoll", "Ragdoll destabilized" );
                StopPreview();
            }
        }
    }

    //-------------------------------------------------------------------------
    // Editor UI
    //-------------------------------------------------------------------------

    void RagdollEditor::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID = 0, bottomLeftDockID = 0, rightDockID = 0, bottomDockID = 0, viewportDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.25f, &leftDockID, &viewportDockID );
        ImGui::DockBuilderSplitNode( leftDockID, ImGuiDir_Up, 0.67f, &leftDockID, &bottomLeftDockID );
        ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Right, 0.33f, &rightDockID, &viewportDockID );
        ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Down, 0.33f, &bottomDockID, &viewportDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Outliner" ).c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Details" ).c_str(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Self Collision" ).c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Preview Controls" ).c_str(), rightDockID );
    }

    void RagdollEditor::DrawToolbar( UpdateContext const& context )
    {
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float const liveDebugButtonWidth = 250;
        ImGui::SameLine( ( availableWidth - liveDebugButtonWidth ) / 2.0f, 0.0f );

        if ( IsPreviewing() )
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_STOP, "Stop Preview", Colors::Red, ImVec2( liveDebugButtonWidth, 0 ) ) )
            {
                StopPreview();
            }
        }
        else
        {
            ImGui::BeginDisabled( !IsResourceLoaded() || IsPreviewingOrWaitingForPreview() );
            if ( ImGuiX::FlatIconButton( EE_ICON_PLAY, "Preview Ragdoll", Colors::Green, ImVec2( liveDebugButtonWidth, 0 ) ) )
            {
                RequestPreview();
            }
            ImGui::EndDisabled();
        }
    }

    void RagdollEditor::ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport )
    {
        if ( IsPreviewing() )
        {
            auto pPhysicsWorld = m_pWorld->GetWorldSystem<PhysicsWorldSystem>()->GetPhysicsWorld();
            Seconds const timeStep = pPhysicsWorld->GetTimeStepInterval();

            ImGui::NewLine();
            ImGuiX::DrawShadowedText(  Colors::Yellow, "Current Physics Time Step: %.6fs", timeStep.ToFloat() );
        }
    }

    bool RagdollEditor::ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport )
    {
        if ( ImGui::MenuItem( "Show Mesh", nullptr, &m_showMesh ) )
        {
            if ( m_pMeshComponent != nullptr )
            {
                m_pMeshComponent->SetVisible( m_showMesh );
            }
        }

        ImGui::SeparatorText( "Editing" );

        ImGui::MenuItem( "Show Skeleton", nullptr, &m_drawSkeleton );
        ImGui::MenuItem( "Show Body Axes", nullptr, &m_drawBodyAxes );
        ImGui::MenuItem( "Show Only Selected Bodies", nullptr, &m_isolateSelectedBody );

        ImGui::SeparatorText( "Preview" );

        ImGui::MenuItem( "Draw Ragdoll", nullptr, &m_drawRagdoll );
        ImGui::MenuItem( "Draw Input Anim Pose", nullptr, &m_drawInputAnimationPose );
        ImGui::MenuItem( "Draw Output Anim Pose", nullptr, &m_drawFinalAnimationPose );

        return true;
    }

    void RagdollEditor::DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        TResourceEditor<RagdollDefinition>::DrawViewportUI( context, pViewport, isFocused );

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        if ( pRagdollDescriptor == nullptr )
        {
            return;
        }

        if ( !m_skeleton.IsLoaded() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( IsPreviewing() )
        {
            DrawViewportUIForPreview( context, pViewport, isFocused );
        }
        else // Editing
        {
            DrawViewportUIForEditing( context, pViewport, isFocused );
        }
    }

    void RagdollEditor::HandleViewportInteractions()
    {
        if ( m_selectedElement.IsSet() && ImGui::IsKeyPressed( ImGuiKey_F ) )
        {
            Transform const selectedElementTransform = GetSelectedElementTransform();
            FocusCameraView( selectedElementTransform.GetTranslation(), 2.0f );
        }
    }

    //-------------------------------------------------------------------------
    // Shared
    //-------------------------------------------------------------------------

    Transform RagdollEditor::GetSelectedElementTransform()
    {
        Transform t;

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        if ( pRagdollDescriptor == nullptr )
        {
            return t;
        }

        //-------------------------------------------------------------------------

        if( !m_selectedElement.IsSet() )
        {
            t = Transform::Identity;
        }
        else if ( m_selectedElement.IsBoneOrBody() )
        {
            t = pRagdollDescriptor->GetBodyTransform( m_skeleton.GetPtr(), m_selectedElement.m_boneID );
        }
        else if ( m_selectedElement.IsJoint() )
        {
            t = pRagdollDescriptor->GetBodyTransform( m_skeleton.GetPtr(), m_selectedElement.m_boneID );
        }
        else if ( m_selectedElement.IsShape() )
        {
            t = pRagdollDescriptor->GetShapeTransform( m_skeleton.GetPtr(), m_selectedElement.m_boneID, m_selectedElement.m_shapeIdx );
        }

        return t;
    }

    //-------------------------------------------------------------------------
    // Editor
    //-------------------------------------------------------------------------

    void RagdollEditor::CreateBody( StringID boneID )
    {
        EE_ASSERT( IsDataFileLoaded() );
        EE_ASSERT( m_skeleton.IsLoaded() );

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        if ( !pRagdollDescriptor->IsValid() || pRagdollDescriptor->GetNumBodies() >= RagdollDefinition::s_maxNumBodies )
        {
            return;
        }

        ScopedDataFileModification const sdm( this );

        EE_ASSERT( pRagdollDescriptor->GetBodyIndexForBoneID( boneID ) == InvalidIndex );

        int32_t const newBodyBoneIdx = m_skeleton->GetBoneIndex( boneID );
        int32_t const numBodies = pRagdollDescriptor->GetNumBodies();
        int32_t bodyInsertionIdx = 0;
        for ( bodyInsertionIdx = 0; bodyInsertionIdx < numBodies; bodyInsertionIdx++ )
        {
            int32_t const existingBodyBoneIdx = m_skeleton->GetBoneIndex( pRagdollDescriptor->m_bodies[bodyInsertionIdx].m_boneID );
            if ( existingBodyBoneIdx > newBodyBoneIdx )
            {
                break;
            }
        }

        // Body
        //-------------------------------------------------------------------------

        pRagdollDescriptor->m_bodies.insert( pRagdollDescriptor->m_bodies.begin() + bodyInsertionIdx, RagdollBodyDefinition() );
        auto& body = pRagdollDescriptor->m_bodies[bodyInsertionIdx];
        body.m_boneID = boneID;

        // Shape
        //-------------------------------------------------------------------------

        CreateShape( bodyInsertionIdx );

        // Joint
        //-------------------------------------------------------------------------

        UpdateJointSetupDueToHierarchyChange();
    }

    void RagdollEditor::DestroyBody( int32_t bodyIdx )
    {
        ScopedDataFileModification const sdm( this );

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() && m_skeleton.IsLoaded() );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < pRagdollDescriptor->GetNumBodies() );
        StringID const boneID = pRagdollDescriptor->m_bodies[bodyIdx].m_boneID;
        pRagdollDescriptor->m_bodies.erase( pRagdollDescriptor->m_bodies.begin() + bodyIdx );

        UpdateJointSetupDueToHierarchyChange();

        // Remove any rules for the body we're deleting
        for ( int32_t i = (int32_t) pRagdollDescriptor->m_disableCollisionRules.size() - 1; i >= 0; i-- )
        {
            if ( pRagdollDescriptor->m_disableCollisionRules[i].m_boneA == boneID || pRagdollDescriptor->m_disableCollisionRules[i].m_boneB == boneID )
            {
                pRagdollDescriptor->m_disableCollisionRules.erase( pRagdollDescriptor->m_disableCollisionRules.begin() + i );
            }
        }
    }

    void RagdollEditor::DestroyChildBodies( int32_t destructionRootBodyIdx )
    {
        ScopedDataFileModification const sdm( this );
        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() && m_skeleton.IsLoaded() );
        EE_ASSERT( destructionRootBodyIdx >= 0 && destructionRootBodyIdx < pRagdollDescriptor->GetNumBodies() );

        int32_t const destructionRootBoneIdx = m_skeleton->GetBoneIndex( pRagdollDescriptor->m_bodies[destructionRootBodyIdx].m_boneID );

        for ( int32_t bodyIdx = 0; bodyIdx < (int32_t) pRagdollDescriptor->m_bodies.size(); bodyIdx++ )
        {
            int32_t const bodyBoneIdx = m_skeleton->GetBoneIndex( pRagdollDescriptor->m_bodies[bodyIdx].m_boneID );
            if ( m_skeleton->IsChildBoneOf( destructionRootBoneIdx, bodyBoneIdx ) )
            {
                DestroyBody( bodyIdx );
                bodyIdx--;
            }
        }
    }

    void RagdollEditor::CreateShape( int32_t bodyIdx )
    {
        ScopedDataFileModification const sdm( this );

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() && m_skeleton.IsLoaded() );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < pRagdollDescriptor->GetNumBodies() );

        auto& body = pRagdollDescriptor->m_bodies[bodyIdx];

        //-------------------------------------------------------------------------

        int32_t const boneIdx = m_skeleton->GetBoneIndex( body.m_boneID );
        Transform bodyTransform = m_skeleton->GetBoneModelSpaceTransform( boneIdx );
        int32_t const firstChildIdx = m_skeleton->GetFirstChildBoneIndex( boneIdx );
        if ( firstChildIdx != InvalidIndex )
        {
            auto childBoneTransform = m_skeleton->GetBoneModelSpaceTransform( firstChildIdx );

            Vector bodyAxis;
            float length = 0.0f;
            ( childBoneTransform.GetTranslation() - bodyTransform.GetTranslation() ).ToDirectionAndLength3( bodyAxis, length );

            // Create a default capsule shape
            //-------------------------------------------------------------------------

            RagdollShapeDefinition& shape = body.m_shapes.emplace_back();

            // See if we need to adjust the default radius
            float const halfLength = length / 2;
            if ( shape.m_radius > halfLength )
            {
                shape.m_radius = halfLength * 2.0f / 3.0f;
            }

            // Calculate the new half-height for the capsule
            shape.m_halfHeight = Math::Max( ( length / 2 ) - shape.m_radius, 0.0f );
            Vector const bodyOffset = bodyAxis * ( shape.m_halfHeight + shape.m_radius );

            // Calculate the offset transform for the shape
            Quaternion const orientation = Quaternion::FromRotationBetweenUnitVectors( Vector::UnitZ, bodyAxis );
            Transform const shapeTransform( orientation, bodyTransform.GetTranslation() + bodyOffset );
            shape.m_offsetTransform = shapeTransform * bodyTransform.GetInverse();
        }
        else // Add default shape
        {
            RagdollShapeDefinition& shape = body.m_shapes.emplace_back();
        }
    }

    bool RagdollEditor::CanDestroyShape( int32_t bodyIdx, int32_t shapeIdx ) const
    {
        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() && m_skeleton.IsLoaded() );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < pRagdollDescriptor->GetNumBodies() );
        EE_ASSERT( shapeIdx >= 0 && shapeIdx < pRagdollDescriptor->m_bodies[bodyIdx].m_shapes.size() );
        return pRagdollDescriptor->m_bodies[bodyIdx].m_shapes.size() > 1;
    }

    void RagdollEditor::DestroyShape( int32_t bodyIdx, int32_t shapeIdx )
    {
        ScopedDataFileModification const sdm( this );

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() && m_skeleton.IsLoaded() );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < pRagdollDescriptor->GetNumBodies() );
        EE_ASSERT( shapeIdx >= 0 && shapeIdx < pRagdollDescriptor->m_bodies[bodyIdx].m_shapes.size() );
        pRagdollDescriptor->m_bodies[bodyIdx].m_shapes.erase( pRagdollDescriptor->m_bodies[bodyIdx].m_shapes.begin() + shapeIdx );
    }

    void RagdollEditor::CopyShapeCollisionSettings( int32_t bodyIdx, int32_t shapeIdx )
    {
        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() && m_skeleton.IsLoaded() );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < pRagdollDescriptor->GetNumBodies() );
        EE_ASSERT( shapeIdx >= 0 && shapeIdx < pRagdollDescriptor->m_bodies[bodyIdx].m_shapes.size() );

        CollisionSettings const settings = pRagdollDescriptor->m_bodies[bodyIdx].m_shapes[shapeIdx].m_collisionSettings;

        ScopedDataFileModification const sdm( this );
        for ( auto& body : pRagdollDescriptor->m_bodies )
        {
            for ( auto& shape : body.m_shapes )
            {
                shape.m_collisionSettings = settings;
            }
        }
    }

    void RagdollEditor::UpdateJointSetupDueToHierarchyChange()
    {
        EE_ASSERT( HasActiveUndoableAction() );

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() && m_skeleton.IsLoaded() );

        int32_t const numBodies = pRagdollDescriptor->GetNumBodies();
        for ( int32_t bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            // If we are the root, we cannot have a joint type set
            if ( pRagdollDescriptor->GetParentBodyIndex( m_skeleton.GetPtr(), bodyIdx ) == InvalidIndex )
            {
                pRagdollDescriptor->m_bodies[bodyIdx].m_joint.m_type = RagdollJointDefinition::Type::None;
            }
            else // Make sure all other bodies have joints set
            {
                if ( pRagdollDescriptor->m_bodies[bodyIdx].m_joint.m_type == RagdollJointDefinition::Type::None )
                {
                    pRagdollDescriptor->m_bodies[bodyIdx].m_joint.m_type = RagdollJointDefinition::Type::Spherical;
                }
            }
        }
    }

    void RagdollEditor::ResetJointTransform( int32_t bodyIdx )
    {
        ScopedDataFileModification const sdm( this );

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() && m_skeleton.IsLoaded() );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < pRagdollDescriptor->GetNumBodies() );

        int32_t const boneIdx = m_skeleton->GetBoneIndex( pRagdollDescriptor->m_bodies[bodyIdx].m_boneID );
        Transform const bodyTransform = m_skeleton->GetBoneModelSpaceTransform( boneIdx );
        pRagdollDescriptor->m_bodies[bodyIdx].m_joint.m_orientation = Quaternion::FromRotationBetweenUnitVectors( Vector::UnitZ, bodyTransform.GetAxisX() );
    }

    void RagdollEditor::ChangeJointType( int32_t bodyIdx, RagdollJointDefinition::Type jointType )
    {
        ScopedDataFileModification const sdm( this );

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() && m_skeleton.IsLoaded() );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < pRagdollDescriptor->GetNumBodies() );
        pRagdollDescriptor->m_bodies[bodyIdx].m_joint.m_type = jointType;
    }

    IReflectedType* RagdollEditor::GetTypeToEditBasedOnSelection() const
    {
        IReflectedType* pTypeToEdit = nullptr;

        if ( m_selectedElement.IsSet() )
        {
            auto pRagdollDescriptor = const_cast<RagdollResourceDescriptor*>( GetDataFile<RagdollResourceDescriptor>() );
            for ( auto& body : pRagdollDescriptor->m_bodies )
            {
                if ( m_selectedElement.m_boneID == body.m_boneID )
                {
                    if ( m_selectedElement.IsShape() )
                    {
                        if ( m_selectedElement.m_shapeIdx < body.m_shapes.size() )
                        {
                            pTypeToEdit = &body.m_shapes[m_selectedElement.m_shapeIdx];
                        }
                    }
                    else if( m_selectedElement.IsJoint() )
                    {
                        if ( body.m_joint.m_type != RagdollJointDefinition::Type::None )
                        {
                            pTypeToEdit = &body.m_joint;
                        }
                    }
                    else
                    {
                        pTypeToEdit = &body;
                    }
                }
            }
        }

        return pTypeToEdit;
    }

    uint64_t RagdollEditor::ConvertRagdollElementToHitTestID( RagdollElement const& element ) const
    {
        if ( !m_skeleton.IsLoaded() || !element.IsSet() )
        {
            return 0;
        }

        uint16_t const type = (uint16_t) element.GetType();
        uint32_t const boneIdx = (uint32_t) m_skeleton->GetBoneIndex( element.m_boneID );
        uint16_t shapeIdx = 0;

        if ( element.IsShape() )
        {
            EE_ASSERT( element.m_shapeIdx >= 0 && element.m_shapeIdx < INT16_MAX );
            shapeIdx = uint16_t( element.m_shapeIdx );
        }

        //-------------------------------------------------------------------------

        uint64_t ID = uint64_t( type ) << 48;
        ID = ID | ( uint64_t( boneIdx ) << 16 );
        ID = ID | uint64_t( shapeIdx );

        return ID;
    }

    RagdollEditor::RagdollElement RagdollEditor::ConvertHitTestIDToRagdollElement( uint64_t hitTestID ) const
    {
        RagdollEditor::RagdollElement element;

        if ( !m_skeleton.IsLoaded() || hitTestID == 0 )
        {
            return element;
        }

        //-------------------------------------------------------------------------

        uint16_t const shapeIdx = uint16_t( hitTestID & 0xFFFF );
        uint32_t const boneIdx = uint32_t( ( hitTestID >> 16 ) & 0xFFFFFFFF );
        uint16_t const type = uint16_t( ( hitTestID >> 48 ) & 0xFFFF );

        //-------------------------------------------------------------------------

        if ( !m_skeleton->IsValidBoneIndex( (int32_t) boneIdx ) )
        {
            return element;
        }

        StringID const boneID = m_skeleton->GetBoneID( (int32_t) boneIdx );
        EE_ASSERT( boneID.IsValid() );

        //-------------------------------------------------------------------------

        auto const elementType = (RagdollElement::Type) type;
        switch ( elementType )
        {
            case RagdollElement::Type::BoneOrBody:
            {
                element.SetBoneOrBody( boneID );
            }
            break;

            case RagdollElement::Type::Joint:
            {
                element.SetJoint( boneID );
            }
            break;

            case RagdollElement::Type::Shape:
            {
                element.SetShape( boneID, shapeIdx );
            }
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }

        return element;
    }

    void RagdollEditor::CreateSkeletonTree()
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

    void RagdollEditor::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    void RagdollEditor::DrawOutlinerWindow( UpdateContext const& context, bool isFocused )
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            ImGui::Text( "Skeleton ID: %s", m_skeleton->GetDataPath().GetFilenameWithoutExtension().c_str() );
            EE_ASSERT( m_pSkeletonTreeRoot != nullptr );
            DrawOutlinerBoneItem( m_pSkeletonTreeRoot );
        }

        if ( isFocused && m_selectedElement.IsSet() && ImGui::IsKeyPressed( ImGuiKey_F ) )
        {
            Transform const selectedElementTransform = GetSelectedElementTransform();
            FocusCameraView( selectedElementTransform.GetTranslation(), 2.0f );
        }
    }

    void RagdollEditor::DrawDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsDataFileLoaded() || !m_skeleton.IsLoaded() )
        {
            return;
        }
        //-------------------------------------------------------------------------

        IReflectedType* pTypeToEdit = GetTypeToEditBasedOnSelection();
        if ( m_propertyGrid.GetEditedType() != pTypeToEdit )
        {
            m_propertyGrid.SetTypeToEdit( pTypeToEdit );
        }

        //-------------------------------------------------------------------------

        m_propertyGrid.UpdateAndDraw();
    }

    void RagdollEditor::DrawSelfCollisionEditorWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsDataFileLoaded() || !m_skeleton.IsLoaded() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        struct Pair
        {
            inline bool operator==( Pair const& rhs ) const
            {
                if ( m_bodyA == rhs.m_bodyA && m_bodyB == rhs.m_bodyB )
                {
                    return true;
                }

                if ( m_bodyA == rhs.m_bodyB && m_bodyB == rhs.m_bodyA )
                {
                    return true;
                }

                return false;
            }

            int32_t     m_bodyA;
            int32_t     m_bodyB;
            bool        m_collides;
        };

        // Generate Options
        //-------------------------------------------------------------------------

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        auto pSkeleton = m_skeleton.GetPtr();
        int32_t const numBodies = pRagdollDescriptor->GetNumBodies();

        TInlineVector<Pair, 100> pairs;

        for ( auto i = 0; i < numBodies; i++ )
        {
            for ( auto j = 0; j < numBodies; j++ )
            {
                if ( i == j )
                {
                    continue;
                }

                if ( pRagdollDescriptor->GetParentBodyIndex( pSkeleton, j ) == i || pRagdollDescriptor->GetParentBodyIndex( pSkeleton, i ) == j )
                {
                    continue;
                }

                Pair p = { i, j, !pRagdollDescriptor->IsCollisionDisabledBetweenBodies( i, j ) };
                VectorEmplaceBackUnique( pairs, p );
            }
        }

        // Draw list
        //-------------------------------------------------------------------------

        m_selfCollisionFilter.UpdateAndDraw();

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 16, 2 ) );
        if ( ImGui::BeginTable( "PT", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders, ImGui::GetContentRegionAvail() ) )
        {
            ImGui::TableSetupColumn( "Body A", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize, 0.5 );
            ImGui::TableSetupColumn( "Body B", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize, 0.5 );
            ImGui::TableSetupColumn( "Collides", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 64 );

            ImGui::TableHeadersRow();

            for ( auto& p : pairs )
            {
                if ( m_selfCollisionFilter.HasFilterSet() )
                {
                    bool const matchesA = m_selfCollisionFilter.MatchesFilter( pRagdollDescriptor->m_bodies[p.m_bodyA].m_boneID.c_str() );
                    bool const matchesB = m_selfCollisionFilter.MatchesFilter( pRagdollDescriptor->m_bodies[p.m_bodyB].m_boneID.c_str() );
                    if ( !matchesA && !matchesB )
                    {
                        continue;
                    }
                }

                //-------------------------------------------------------------------------

                InlineString const str( InlineString::CtorSprintf(), "%d_%d", p.m_bodyA, p.m_bodyB );
                ImGui::PushID( str.c_str() );

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "%s", pRagdollDescriptor->m_bodies[p.m_bodyA].m_boneID.c_str() );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "%s", pRagdollDescriptor->m_bodies[p.m_bodyB].m_boneID.c_str() );

                ImGui::TableNextColumn();
                ImGui::BeginDisabled( IsPreviewing() );
                if ( ImGui::Checkbox( "##", &p.m_collides ) )
                {
                    RagdollDisableCollisionRule rule;
                    rule.m_boneA = pRagdollDescriptor->m_bodies[p.m_bodyA].m_boneID;
                    rule.m_boneB = pRagdollDescriptor->m_bodies[p.m_bodyB].m_boneID;

                    ScopedDataFileModification const sdm( this );
                    if ( p.m_collides )
                    {
                        pRagdollDescriptor->m_disableCollisionRules.erase_first( rule );
                    }
                    else
                    {
                        VectorEmplaceBackUnique( pRagdollDescriptor->m_disableCollisionRules, rule );
                    }
                }
                ImGui::EndDisabled();

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    }

    ImRect RagdollEditor::DrawOutlinerBoneItem( BoneInfo* pBone )
    {
        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() );
        EE_ASSERT( m_skeleton.IsLoaded() );
        EE_ASSERT( pBone != nullptr );

        //-------------------------------------------------------------------------

        StringID const currentBoneID = m_skeleton->GetBoneID( pBone->m_boneIdx );
        int32_t const bodyIdx = pRagdollDescriptor->GetBodyIndexForBoneID( currentBoneID );
        bool const hasBody = bodyIdx != InvalidIndex;

        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( pBone->m_isExpanded );
        int32_t treeNodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        bool const isSelected = m_selectedElement.IsBoneOrBody() && m_selectedElement.m_boneID == currentBoneID;
        if ( isSelected )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        InlineString boneLabel;
        if ( hasBody )
        {
            boneLabel.sprintf( "%d. %s %s", pBone->m_boneIdx, EE_ICON_ACCOUNT, currentBoneID.c_str() );
        }
        else
        {
            boneLabel.sprintf( "%d. %s", pBone->m_boneIdx, currentBoneID.c_str() );
        }

        if ( hasBody )
        {
            ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::MediumBoldItalic : ImGuiX::Font::Medium, Colors::HotPink );
            pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
        }
        else
        {
            pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
        }

        // Context Menu
        if ( ImGui::BeginPopupContextItem() )
        {
            if ( hasBody )
            {
                if ( ImGui::MenuItem( "Create shape" ) )
                {
                    m_commandStack.PushCommand( [this, bodyIdx] () { CreateShape( bodyIdx ); } );
                }

                if ( ImGui::MenuItem( "Destroy Body" ) )
                {
                    m_commandStack.PushCommand( [this, bodyIdx] () { DestroyBody( bodyIdx ); } );
                }

                if ( ImGui::MenuItem( "Destroy All Child Bodies" ) )
                {
                    m_commandStack.PushCommand( [this, bodyIdx] () { DestroyChildBodies( bodyIdx ); } );
                }
            }
            else
            {
                if ( pRagdollDescriptor->GetNumBodies() >= RagdollDefinition::s_maxNumBodies )
                {
                    ImGui::Text( "Max number of bodies reached" );
                }
                else
                {
                    if ( ImGui::MenuItem( "Create Body" ) )
                    {
                        m_commandStack.PushCommand( [this, currentBoneID] () { CreateBody( currentBoneID ); } );
                    }
                }
            }

            ImGui::EndPopup();
        }

        // Handle bone/body selection
        if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenBlockedByPopup ) )
        {
            if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
            {
                m_selectedElement.SetBoneOrBody( currentBoneID );
            }
        }

        //-------------------------------------------------------------------------

        ImRect const nodeRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

        if ( pBone->m_isExpanded )
        {
            if ( hasBody )
            {
                DrawOutlinerBodyItems( bodyIdx );
            }

            //-------------------------------------------------------------------------

            ImU32 const TreeLineColor = ImGui::GetColorU32( ImGuiCol_TextDisabled );
            float const SmallOffsetX = 2;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
            verticalLineStart.x += SmallOffsetX;
            ImVec2 verticalLineEnd = verticalLineStart;

            for ( BoneInfo* pChild : pBone->m_children )
            {
                const float HorizontalTreeLineSize = 4.0f;
                const ImRect childRect = DrawOutlinerBoneItem( pChild );
                const float midpoint = ( childRect.Min.y + childRect.Max.y ) / 2.0f;
                drawList->AddLine( ImVec2( verticalLineStart.x, midpoint ), ImVec2( verticalLineStart.x + HorizontalTreeLineSize, midpoint ), TreeLineColor );
                verticalLineEnd.y = midpoint;
            }

            drawList->AddLine( verticalLineStart, verticalLineEnd, TreeLineColor );
            ImGui::TreePop();
        }

        return nodeRect;
    }

    void RagdollEditor::DrawOutlinerBodyItems( int32_t bodyIdx )
    {
        constexpr float const horizontalTreeLineSize = 4.0f;

        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr && pRagdollDescriptor->IsValid() );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < pRagdollDescriptor->m_bodies.size() );

        InlineString itemLabel;

        ImGui::Indent();
        auto const& body = pRagdollDescriptor->m_bodies[bodyIdx];

        // Joint
        //-------------------------------------------------------------------------

        if ( body.m_joint.m_type != RagdollJointDefinition::Type::None )
        {
            ImU32 const treeLineColor = ImGui::GetColorU32( ImGuiCol_TextDisabled );
            float const smallOffsetX = 2;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
            verticalLineStart.x += smallOffsetX;
            ImVec2 verticalLineEnd = verticalLineStart;

            switch ( body.m_joint.m_type )
            {
                case RagdollJointDefinition::Type::Spherical:
                {
                    itemLabel.sprintf( EE_ICON_CONE" Spherical Joint (%s)", body.m_boneID.c_str() );
                }
                break;

                case RagdollJointDefinition::Type::Revolute:
                {
                    itemLabel.sprintf( EE_ICON_ANGLE_ACUTE" Revolute Joint (%s)", body.m_boneID.c_str() );
                }
                break;

                case RagdollJointDefinition::Type::None:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }

            int32_t treeNodeFlags = 0;
            bool const isSelected = m_selectedElement.IsJoint() && m_selectedElement.m_boneID == body.m_boneID;
            if ( isSelected )
            {
                treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
            }

            // Item
            {
                ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::MediumBoldItalic : ImGuiX::Font::Medium, Colors::Gold );
                ImGui::SetNextItemOpen( true );
                if ( ImGui::TreeNodeEx( itemLabel.c_str(), treeNodeFlags ) )
                {
                    const ImRect childRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
                    const float midpoint = ( childRect.Min.y + childRect.Max.y ) / 2.0f;
                    drawList->AddLine( ImVec2( verticalLineStart.x, midpoint ), ImVec2( verticalLineStart.x + horizontalTreeLineSize, midpoint ), treeLineColor );
                    verticalLineEnd.y = midpoint;
                    drawList->AddLine( verticalLineStart, verticalLineEnd, treeLineColor );
                    ImGui::TreePop();
                }
            }

            // Context Menu
            if ( ImGui::BeginPopupContextItem() )
            {
                if ( body.m_joint.m_type != RagdollJointDefinition::Type::Revolute )
                {
                    if ( ImGui::MenuItem( EE_ICON_ANGLE_ACUTE" Switch joint to Revolute" ) )
                    {
                        m_commandStack.PushCommand( [this, bodyIdx] () { ChangeJointType( bodyIdx, RagdollJointDefinition::Type::Revolute ); } );
                    }
                }

                if ( body.m_joint.m_type != RagdollJointDefinition::Type::Spherical )
                {
                    if ( ImGui::MenuItem( EE_ICON_CONE" Switch joint to Spherical" ) )
                    {
                        m_commandStack.PushCommand( [this, bodyIdx] () { ChangeJointType( bodyIdx, RagdollJointDefinition::Type::Spherical ); } );
                    }
                }

                if ( ImGui::MenuItem( "Reset Transform" ) )
                {
                    m_commandStack.PushCommand( [this, bodyIdx] () { ResetJointTransform( bodyIdx ); } );
                }

                ImGui::EndPopup();
            }

            // Handle joint selection
            if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenBlockedByPopup ) )
            {
                if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
                {
                    m_selectedElement.SetJoint( body.m_boneID );
                }
            }
        }

        // Shapes
        //-------------------------------------------------------------------------

        int32_t const numShapes = (int32_t) body.m_shapes.size();
        for ( int32_t i = 0; i < numShapes; i++ )
        {
            auto const& shape = body.m_shapes[i];

            ImU32 const treeLineColor = ImGui::GetColorU32( ImGuiCol_TextDisabled );
            float const smallOffsetX = 2;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
            verticalLineStart.x += smallOffsetX;
            ImVec2 verticalLineEnd = verticalLineStart;

            switch ( shape.m_type )
            {
                case RagdollShapeDefinition::Type::Capsule:
                {
                    itemLabel.sprintf( EE_ICON_PILL" Capsule (%s)##%d", body.m_boneID.c_str(), i );
                }
                break;

                case RagdollShapeDefinition::Type::Box:
                {
                    itemLabel.sprintf( EE_ICON_CUBE" Box (%s)##%d", body.m_boneID.c_str(), i );
                }
                break;

                case RagdollShapeDefinition::Type::Sphere:
                {
                    itemLabel.sprintf( EE_ICON_SPHERE" Sphere (%s)##%d", body.m_boneID.c_str(), i );
                }
                break;
            }

            int32_t treeNodeFlags = 0;
            bool const isSelected = m_selectedElement.IsShape() && m_selectedElement.m_boneID == body.m_boneID && m_selectedElement.m_shapeIdx == i;
            if ( isSelected )
            {
                treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
            }

            // Item
            {
                ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::MediumBoldItalic : ImGuiX::Font::Medium, Colors::LightSkyBlue );
                ImGui::SetNextItemOpen( true );
                if ( ImGui::TreeNodeEx( itemLabel.c_str(), treeNodeFlags ) )
                {
                    const ImRect childRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
                    const float midpoint = ( childRect.Min.y + childRect.Max.y ) / 2.0f;
                    drawList->AddLine( ImVec2( verticalLineStart.x, midpoint ), ImVec2( verticalLineStart.x + horizontalTreeLineSize, midpoint ), treeLineColor );
                    verticalLineEnd.y = midpoint;
                    drawList->AddLine( verticalLineStart, verticalLineEnd, treeLineColor );
                    ImGui::TreePop();
                }
            }

            // Context Menu
            
            if ( ImGui::BeginPopupContextItem() )
            {
                if ( ImGui::MenuItem( "Copy collision settings to all shapes" ) )
                {
                    m_commandStack.PushCommand( [this, bodyIdx, i] () { CopyShapeCollisionSettings( bodyIdx, i ); } );
                }

                if ( CanDestroyShape( bodyIdx, i ) )
                {
                    if ( ImGui::MenuItem( "Destroy shape" ) )
                    {
                        m_commandStack.PushCommand( [this, bodyIdx, i] () { DestroyShape( bodyIdx, i ); } );
                    }
                }

                ImGui::EndPopup();
            }

            // Handle shape selection
            if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenBlockedByPopup ) )
            {
                if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
                {
                    m_selectedElement.SetShape( body.m_boneID, i );
                }
            }
        }

        ImGui::Unindent();
    }

    void RagdollEditor::DrawViewportUIForEditing( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        if ( pRagdollDescriptor == nullptr )
        {
            return;
        }

        auto drawingCtx = GetDebugDrawContext();

        // Draw skeleton
        if ( m_drawSkeleton )
        {
            m_skeleton->DrawDebug( drawingCtx, Transform::Identity, Animation::Skeleton::LOD::High );
        }

        //-------------------------------------------------------------------------

        int32_t const numBodies = pRagdollDescriptor->GetNumBodies();
        for ( auto bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
        {
            auto& body = pRagdollDescriptor->m_bodies[bodyIdx];

            if ( m_isolateSelectedBody )
            {
                if ( m_selectedElement.IsBoneOrBody() && body.m_boneID != m_selectedElement.m_boneID )
                {
                    continue;
                }
                else if ( m_selectedElement.IsJoint() )
                {
                    int32_t const parentBodyIdx = pRagdollDescriptor->GetParentBodyIndex( m_skeleton.GetPtr(), pRagdollDescriptor->GetBodyIndex( m_selectedElement.m_boneID ) );
                    bool const isJointBody = body.m_boneID == m_selectedElement.m_boneID;
                    bool const isJointParentBody = ( bodyIdx == parentBodyIdx );
                    if ( !isJointBody && !isJointParentBody )
                    {
                        continue;
                    }
                }
            }

            // Draw Shapes
            //-------------------------------------------------------------------------

            Transform const bodyTransform = pRagdollDescriptor->GetBodyTransform( m_skeleton.GetPtr(), bodyIdx );
            for ( int32_t shapeIdx = 0; shapeIdx < body.m_shapes.size(); shapeIdx++ )
            {
                RagdollElement element;
                element.SetShape( body.m_boneID, shapeIdx );

                uint64_t const hitTestID = ConvertRagdollElementToHitTestID( element );
                drawingCtx.SetHitTestID( hitTestID );
                RagdollShapeDefinition const& shape = body.m_shapes[shapeIdx];
                shape.DrawDebug( drawingCtx, bodyTransform, m_selectedElement.IsJoint() ? Colors::LightGreen.GetAlphaVersion( 0.75f ) : Colors::LightGreen );
                drawingCtx.ClearHitTestID();
            }

            // Draw joint
            //-------------------------------------------------------------------------

            if ( bodyIdx > 0 )
            {
                auto& joint = body.m_joint;
                if ( joint.m_type != RagdollJointDefinition::Type::None )
                {
                    int32_t const parentBodyIdx = pRagdollDescriptor->GetParentBodyIndex( m_skeleton.GetPtr(), bodyIdx );
                    EE_ASSERT( parentBodyIdx != InvalidIndex );
                    Transform const transformA = pRagdollDescriptor->GetBodyTransform( m_skeleton.GetPtr(), parentBodyIdx );
                    Transform const transformB = pRagdollDescriptor->GetBodyTransform( m_skeleton.GetPtr(), bodyIdx );

                    joint.FinalizeJoint( transformA, transformB );

                    RagdollElement element;
                    element.SetJoint( body.m_boneID );

                    uint64_t const hitTestID = ConvertRagdollElementToHitTestID( element );
                    drawingCtx.SetHitTestID( hitTestID );
                    joint.DrawDebug( drawingCtx, transformA, transformB );
                    drawingCtx.ClearHitTestID();
                }
            }
        }

        // Gizmo
        //-------------------------------------------------------------------------

        if ( m_selectedElement.IsSet() )
        {
            auto TryGetGizmoTransform = [this, pRagdollDescriptor]( Transform &outTransform ) -> bool
            {
                if( m_selectedElement.IsJoint() )
                {
                    int32_t const bodyIdx = pRagdollDescriptor->GetBodyIndex( m_selectedElement.m_boneID );
                    outTransform = pRagdollDescriptor->GetBodyTransform( m_skeleton.GetPtr(), m_selectedElement.m_boneID );
                    outTransform.AddRotation( pRagdollDescriptor->m_bodies[bodyIdx].m_joint.m_orientation );
                    return true;
                }
                else if ( m_selectedElement.IsShape() )
                {
                    outTransform = pRagdollDescriptor->GetShapeTransform( m_skeleton.GetPtr(), m_selectedElement.m_boneID, m_selectedElement.m_shapeIdx );
                    return true;
                }

                return false;
            };

            auto ApplyGizmoResult = [this, pRagdollDescriptor] ( ImGuiX::Gizmo::Result const& result )
            {
                if ( result.IsScale() )
                {
                    EE_ASSERT( m_selectedElement.IsShape() );
                    pRagdollDescriptor->ScaleShape( m_selectedElement.m_boneID, m_selectedElement.m_shapeIdx, result.m_delta );
                }
                else
                {
                    if ( m_selectedElement.IsShape() )
                    {
                        Transform shapeTransform = pRagdollDescriptor->GetShapeTransform( m_skeleton.GetPtr(), m_selectedElement.m_boneID, m_selectedElement.m_shapeIdx );
                        shapeTransform = result.GetModifiedTransform( shapeTransform );
                        pRagdollDescriptor->SetShapeTransform( m_skeleton.GetPtr(), m_selectedElement.m_boneID, m_selectedElement.m_shapeIdx, shapeTransform );
                    }
                    else if( m_selectedElement.IsJoint() )
                    {
                        if ( result.m_deltaType == ImGuiX::Gizmo::ResultDeltaType::Rotation )
                        {
                            int32_t const bodyIdx = pRagdollDescriptor->GetBodyIndex( m_selectedElement.m_boneID );
                            Quaternion delta( result.m_delta );
                            Quaternion newR = pRagdollDescriptor->m_bodies[bodyIdx].m_joint.m_orientation * delta;

                            EE_ASSERT( !newR.ToVector().IsZero4() );
                            pRagdollDescriptor->m_bodies[bodyIdx].m_joint.m_orientation = newR.GetNormalized();
                        }
                    }
                }
            };

            Transform gizmoTransform;
            bool const wasAbleToGetGizmoTransform = TryGetGizmoTransform( gizmoTransform );

            if ( wasAbleToGetGizmoTransform )
            {
                m_gizmo.SetCoordinateSystemSpace( CoordinateSpace::World );

                if ( m_selectedElement.IsShape() )
                {
                    m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, true );
                    m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowNonUniformScale, pRagdollDescriptor->GetShapeType( m_selectedElement.m_boneID, m_selectedElement.m_shapeIdx ) != RagdollShapeDefinition::Type::Sphere );
                }
                else
                {
                    m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );
                }

                if ( m_selectedElement.IsJoint() )
                {
                    if ( !m_gizmo.IsManipulating() )
                    {
                        m_gizmo.SetMode( ImGuiX::Gizmo::Mode::Rotation );
                    }
                }

                auto const gizmoResult = m_gizmo.Draw( gizmoTransform.GetTranslation(), gizmoTransform.GetRotation(), *pViewport );
                switch ( gizmoResult.m_state )
                {
                    case ImGuiX::GizmoState::StartedManipulating:
                    {
                        BeginDataFileModification();
                        ApplyGizmoResult( gizmoResult );
                    }
                    break;

                    case ImGuiX::GizmoState::Manipulating:
                    {
                        ApplyGizmoResult( gizmoResult );
                    }
                    break;

                    case ImGuiX::GizmoState::StoppedManipulating:
                    {
                        ApplyGizmoResult( gizmoResult );
                        EndDataFileModification();
                    }
                    break;

                    default:
                    break;
                }

                if ( isFocused )
                {
                    if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
                    {
                        m_gizmo.SwitchToNextMode();
                        if ( m_gizmo.GetMode() == ImGuiX::Gizmo::Mode::Scale )
                        {
                            m_gizmo.SetCoordinateSystemSpace( CoordinateSpace::Local );
                        }
                        else
                        {
                            m_gizmo.SetCoordinateSystemSpace( CoordinateSpace::World );
                        }
                    }
                }
            }
            else
            {
                m_gizmo.Reset();
            }
        }

        // Selection
        //-------------------------------------------------------------------------

        if ( !m_gizmo.IsManipulating() && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
            auto const& pd = GetPickingData();
            PickingID const pickingID = GetPickingID();

            if ( pickingID.IsSet() && !pickingID.HasSecondaryID() )
            {
                RagdollElement pickedElement = ConvertHitTestIDToRagdollElement( pickingID.m_primaryID );
                if ( pickedElement.IsSet() )
                {
                    m_selectedElement = pickedElement;
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    // Preview
    //-------------------------------------------------------------------------

    void RagdollEditor::CreatePreviewEntity()
    {
        EE_ASSERT( IsDataFileLoaded() );
        EE_ASSERT( m_skeleton.IsLoaded() );

        Log log;

        // Load resource descriptor for skeleton to get the preview mesh
        auto resourceDescPath = GetFileSystemPath( m_skeleton.GetDataPath() );
        Animation::SkeletonResourceDescriptor skeletonResourceDesc;
        bool const result = Resource::ResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, log, resourceDescPath, skeletonResourceDesc );
        EE_ASSERT( result );

        // Create preview entity
        ResourceID const previewMeshResourceID( skeletonResourceDesc.m_previewMesh.GetDataPath() );
        if ( previewMeshResourceID.IsValid() )
        {
            m_pMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
            m_pMeshComponent->SetSkeleton( m_skeleton.GetResourceID() );
            m_pMeshComponent->SetMesh( previewMeshResourceID );
            m_pMeshComponent->SetVisible( m_showMesh );

            m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
            m_pPreviewEntity->AddComponent( m_pMeshComponent );
            AddEntityToWorld( m_pPreviewEntity );
        }
    }

    void RagdollEditor::DestroyPreviewEntity()
    {
        if ( m_pPreviewEntity != nullptr )
        {
            DestroyEntityInWorld( m_pPreviewEntity );
            m_pMeshComponent = nullptr;
        }
    }

    void RagdollEditor::DrawPreviewControlsWindow( UpdateContext const& context, bool isFocused )
    {
        auto pRagdollDescriptor = GetDataFile<RagdollResourceDescriptor>();
        EE_ASSERT( pRagdollDescriptor != nullptr );

        ImGui::BeginDisabled( !IsResourceLoaded() );

        //-------------------------------------------------------------------------
        // General Settings
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Start Transform" ) )
        {
            ImGuiX::InputTransform( "StartTransform", m_previewStartTransform, -1.0f );

            if ( ImGui::Button( EE_ICON_RESTART" Reset Start Transform", ImVec2( -1, 0 ) ) )
            {
                m_previewStartTransform = Transform::Identity;
            }
        }

        //-------------------------------------------------------------------------
        // Ragdoll
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Ragdoll Settings" ) )
        {
            if ( ImGui::BeginTable( "GRID", 2, ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable ) )
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Gravity Scale" );

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::SliderFloat( "##Gravity", &m_gravityScale, 0.0f, 1.0f ) )
                {
                    if ( IsPreviewing() )
                    {
                        m_pRagdoll->SetGravityScale( m_gravityScale );
                    }
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Enable Pose Following" );

                ImGui::TableNextColumn();
                if ( ImGui::Checkbox( "##EPF", &m_enablePoseFollowing ) )
                {
                    if ( IsPreviewing() )
                    {
                        m_pRagdoll->SetPoseFollowing( m_enablePoseFollowing );
                    }
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Blend Weight" );

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##PhysicsWeight", &m_physicsBlendWeight, 0, 1, "%.3f", ImGuiSliderFlags_NoInput );

                ImGui::EndTable();
            }

            ImGui::BeginDisabled( !IsPreviewing() );
            if ( ImGui::Button( EE_ICON_RESTART" Reset Ragdoll State", ImVec2( -1, 0 ) ) )
            {
                m_pRagdoll->Update( 0.0f, m_previewStartTransform, m_pInputPose );
                m_pMeshComponent->SetWorldTransform( m_previewStartTransform );
            }
            ImGui::EndDisabled();
        }

        //-------------------------------------------------------------------------
        // Animation
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Animation" ) )
        {
            if ( ImGui::BeginTable( "GRID", 2, ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable ) )
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Preview Anim" );

                ImGui::TableNextColumn();
                if ( m_animPicker.UpdateAndDraw() )
                {
                    // If we need to change resource ID, switch IDs
                    ResourceID const selectedResourceID = m_animPicker.GetResourceID();
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

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Enable Looping" );

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::Checkbox( "##Enable Looping", &m_enableAnimationLooping );
                

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Apply Root Motion" );

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::Checkbox( "##Apply Root Motion", &m_applyRootMotion );

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Auto Start" );

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::Checkbox( "##Auto Start", &m_autoPlayAnimation );

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Playback" );

                ImGui::TableNextColumn();

                bool const hasValidAndLoadedPreviewAnim = m_previewAnimation.IsSet() && m_previewAnimation.IsLoaded();
                ImGui::BeginDisabled( !IsPreviewing() || !hasValidAndLoadedPreviewAnim );

                ImVec2 const buttonSize( ImGuiX::Style::s_iconButtonWidth, 0 );
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
                float currentAnimTime = m_animTime.ToFloat();
                if ( ImGui::SliderFloat( "##AnimTime", &currentAnimTime, 0, 1, "%.3f", ImGuiSliderFlags_NoInput ) )
                {
                    m_isPlayingAnimation = false;
                    m_animTime = currentAnimTime;
                }

                //-------------------------------------------------------------------------

                ImGui::EndTable();
            }
        }

        //-------------------------------------------------------------------------
        // Manipulation Controls
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Manipulation" ) )
        {
            ImGui::TextWrapped( "Use CTRL + left click to apply an impulse to the ragdoll" );

            if ( ImGui::BeginTable( "GRID", 2, ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable ) )
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Impulse Strength" );

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##IS", &m_impulseStrength, 0.1f, 1000.0f );
                ImGuiX::ItemTooltip( "Applied impulse strength" );

                ImGui::EndTable();
            }
        }

        ImGui::EndDisabled();
    }

    void RagdollEditor::CalculateInputPose( Seconds deltaTime )
    {
        // Update animation time and sample pose
        Percentage const previousAnimTime = m_animTime;
        bool const hasValidAndLoadedPreviewAnim = m_previewAnimation.IsSet() && m_previewAnimation.IsLoaded();
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

                if ( deltaTime > 0 && m_applyRootMotion )
                {
                    Transform const& WT = m_pMeshComponent->GetWorldTransform();
                    Transform const RMD = m_previewAnimation->GetRootMotionDelta( previousAnimTime, m_animTime );
                    m_pMeshComponent->SetWorldTransform( RMD * WT );
                }
            }

            m_previewAnimation->GetPose( m_animTime, m_pInputPose );
        }
        else
        {
            m_pInputPose->Reset( Animation::Pose::Init::ReferencePose );
        }

        m_pInputPose->CalculateModelSpaceTransforms();
    }

    void RagdollEditor::OnPreviewStarted()
    {
        EE_ASSERT( m_pRagdoll == nullptr && IsResourceLoaded() );
        EE_ASSERT( m_skeleton.IsLoaded() );

        //-------------------------------------------------------------------------

        if ( m_autoPlayAnimation )
        {
            m_animTime = 0.0f;
            m_isPlayingAnimation = true;
        }

        m_pInputPose = EE::New<Animation::Pose>( m_skeleton.GetPtr() );
        m_pFinalPose = EE::New<Animation::Pose>( m_skeleton.GetPtr() );

        m_pMeshComponent->SetWorldTransform( m_previewStartTransform );

        CalculateInputPose( 0.0f );

        //-------------------------------------------------------------------------

        RagdollDefinition const* pRagdollDefinition = GetPreviewResource<RagdollDefinition>();
        auto pPhysicsWorld = m_pWorld->GetWorldSystem<PhysicsWorldSystem>()->GetPhysicsWorld();
        m_pRagdoll = EE::New<Ragdoll>( pPhysicsWorld, pRagdollDefinition );
        m_pRagdoll->SetGravityScale( m_gravityScale );
        m_pRagdoll->SetPoseFollowing( m_enablePoseFollowing );

        m_pRagdoll->Update( 0.0f, m_previewStartTransform, m_pInputPose, true );
    }

    void RagdollEditor::OnPreviewStopped()
    {
        EE_ASSERT( m_pRagdoll != nullptr );

        if ( m_isManipulatingRagdoll )
        {
            DestroyRagdollManipulationBody();
        }

        //-------------------------------------------------------------------------

        if ( m_pMeshComponent->HasMeshResourceSet() )
        {
            m_pMeshComponent->ResetPose();
            m_pMeshComponent->FinalizePose();
        }

        m_pMeshComponent->SetWorldTransform( Transform::Identity );

        EE::Delete( m_pRagdoll );
        EE::Delete( m_pInputPose );
        EE::Delete( m_pFinalPose );
    }

    void RagdollEditor::CreateRagdollManipulationBody( b3RayResult const& result )
    {
        EE_ASSERT( !m_isManipulatingRagdoll );

        auto pPhysicsWorld = m_pWorld->GetWorldSystem<PhysicsWorldSystem>()->GetPhysicsWorld();
        b3WorldId const worldID = pPhysicsWorld->GetWorldID();
        ScopedWriteLock const sl( pPhysicsWorld );

        b3BodyDef bodyDef = b3DefaultBodyDef();
        bodyDef.type = b3_kinematicBody;
        bodyDef.position = result.point;
        bodyDef.enableSleep = false;
        m_manipulationBodyID = b3CreateBody( worldID, &bodyDef );

        b3BodyId const hitBodyID = b3Shape_GetBody( result.shapeId );

        b3MotorJointDef jointDef = b3DefaultMotorJointDef();
        jointDef.base.bodyIdA = m_manipulationBodyID;
        jointDef.base.bodyIdB = hitBodyID;
        jointDef.base.localFrameB.p = b3Body_GetLocalPoint( hitBodyID, result.point );
        jointDef.linearHertz = 7.5f;
        jointDef.linearDampingRatio = 1.0f;

        b3MassData massData = b3Body_GetMassData( hitBodyID );
        float const g = b3Length( b3World_GetGravity( worldID ) );
        float const mg = massData.mass * g;
        jointDef.maxSpringForce = 100.0f * mg;

        if ( massData.mass > 0.0f )
        {
            // This acts like angular friction
            float const trace = massData.inertia.cx.x + massData.inertia.cy.y + massData.inertia.cz.z;
            float const lever = Math::Sqrt( trace / ( 3.0f * massData.mass ) );
            jointDef.maxVelocityTorque = 0.5f * lever * mg;
        }

        m_manipulationJointID = b3CreateMotorJoint( worldID, &jointDef );

        m_manipulationPlanePoint = FromBox3D( result.point );
        m_isManipulatingRagdoll = true;
    }

    void RagdollEditor::DestroyRagdollManipulationBody()
    {
        EE_ASSERT( m_isManipulatingRagdoll );

        auto pPhysicsWorld = m_pWorld->GetWorldSystem<PhysicsWorldSystem>()->GetPhysicsWorld();
        ScopedWriteLock const sl( pPhysicsWorld );

        if ( b3Joint_IsValid( m_manipulationJointID ) )
        {
            b3DestroyJoint( m_manipulationJointID, false );
            m_manipulationJointID = {};
        }

        if ( b3Body_IsValid( m_manipulationBodyID ) )
        {
            b3DestroyBody( m_manipulationBodyID );
            m_manipulationBodyID = {};
        }

        m_isManipulatingRagdoll = false;
    }

    void RagdollEditor::UpdateRagdollManipulationBody( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        auto pPhysicsWorld = m_pWorld->GetWorldSystem<PhysicsWorldSystem>()->GetPhysicsWorld();

        //-------------------------------------------------------------------------

        if ( isFocused && ImGui::IsMouseDragging( ImGuiMouseButton_Left ) )
        {
            Float2 const mouseViewportPos = ImGui::GetMousePos();
            Vector const nearPointWS = pViewport->ScreenSpaceToWorldSpaceNearPlane( mouseViewportPos );
            Vector const farPointWS = pViewport->ScreenSpaceToWorldSpaceFarPlane( mouseViewportPos );

            if ( !m_isManipulatingRagdoll )
            {
                b3WorldId const worldID = pPhysicsWorld->GetWorldID();
                b3QueryFilter const filter = { .categoryBits = 1, .maskBits = ~0llu };
                b3RayResult const result = b3World_CastRayClosest( worldID, ToBox3D( nearPointWS ), ToBox3D( farPointWS - nearPointWS ), filter );
                if ( result.hit )
                {
                    CreateRagdollManipulationBody( result );
                }
            }
            else if ( b3Body_IsValid( m_manipulationBodyID ) )
            {
                Plane const manipulationPlane = Plane::FromNormalAndPoint( pViewport->GetViewForwardDirection(), m_manipulationPlanePoint );
                auto const result = Math::IntersectRayPlane( nearPointWS, ( farPointWS - nearPointWS ).GetNormalized3(), manipulationPlane );
                if ( result )
                {
                    ScopedWriteLock const sl( pPhysicsWorld );
                    Transform targetTransform = Transform( Quaternion::Identity, result.m_intersectionPoint );
                    b3Body_SetTargetTransform( m_manipulationBodyID, ToBox3D( targetTransform ), context.GetDeltaTime(), true );

                    auto drawingCtx = GetDebugDrawContext();
                    drawingCtx.DrawSphere( targetTransform, s_manipulationBodyRadius, Colors::Red );
                }
            }
        }
        else if( m_isManipulatingRagdoll )
        {
            DestroyRagdollManipulationBody();
        }
    }

    void RagdollEditor::DrawViewportUIForPreview( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        auto drawingCtx = GetDebugDrawContext();

        Transform previewWorldTransform = m_pMeshComponent->GetWorldTransform();
        auto const gizmoResult = m_gizmo.Draw( previewWorldTransform.GetTranslation(), previewWorldTransform.GetRotation(), *pViewport );
        if ( gizmoResult.m_state == ImGuiX::GizmoState::Manipulating )
        {
            gizmoResult.ApplyResult( previewWorldTransform );
            m_pMeshComponent->SetWorldTransform( previewWorldTransform );
        }

        //-------------------------------------------------------------------------

        if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
            // Unproject mouse into viewport
            Float2 mouseViewportPos = ImGui::GetMousePos();
            Vector const nearPointWS = pViewport->ScreenSpaceToWorldSpaceNearPlane( mouseViewportPos );
            Vector const farPointWS = pViewport->ScreenSpaceToWorldSpaceFarPlane( mouseViewportPos );

            // Impulse
            if ( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) || ImGui::IsKeyDown( ImGuiKey_RightCtrl ) )
            {
                m_pRagdoll->ApplyImpulse( nearPointWS, ( farPointWS - nearPointWS ).GetNormalized3(), m_impulseStrength );
            }
        }

        //-------------------------------------------------------------------------

        UpdateRagdollManipulationBody( context, pViewport, isFocused );
    }
}

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class RagdollJointDefinitionEditingRules : public PG::TTypeEditingRules<RagdollJointDefinition>
    {
        using PG::TTypeEditingRules<RagdollJointDefinition>::TTypeEditingRules;

        virtual HiddenState IsHidden( StringID const& propertyID ) override
        {
            if ( propertyID == StringID( "m_enableHingeLimit" ) || propertyID == StringID( "m_hingeLowerLimit" ) || propertyID == StringID( "m_hingeUpperLimit" ) )
            {
                if ( m_pTypeInstance->m_type != RagdollJointDefinition::Type::Revolute )
                {
                    return HiddenState::Hidden;
                }
            }

            if ( propertyID == StringID( "m_enableTwistLimit" ) || propertyID == StringID( "m_twistLowerLimit" ) || propertyID == StringID( "m_twistUpperLimit" ) || propertyID == StringID( "m_enableConeLimit" )  || propertyID == StringID( "m_coneAngle" ) )
            {
                if ( m_pTypeInstance->m_type != RagdollJointDefinition::Type::Spherical )
                {
                    return HiddenState::Hidden;
                }
            }

            return HiddenState::Unhandled;
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( RagdollJointDefinition, RagdollJointDefinitionEditingRules );


    class RagdollShapeDefinitionEditingRules : public PG::TTypeEditingRules<RagdollShapeDefinition>
    {
        using PG::TTypeEditingRules<RagdollShapeDefinition>::TTypeEditingRules;

        virtual HiddenState IsHidden( StringID const& propertyID ) override
        {
            if ( propertyID == StringID( "m_radius" )  )
            {
                if ( m_pTypeInstance->m_type != RagdollShapeDefinition::Type::Capsule && m_pTypeInstance->m_type != RagdollShapeDefinition::Type::Sphere )
                {
                    return HiddenState::Hidden;
                }
            }

            if ( propertyID == StringID( "m_halfHeight" ) )
            {
                if ( m_pTypeInstance->m_type != RagdollShapeDefinition::Type::Capsule )
                {
                    return HiddenState::Hidden;
                }
            }

            if ( propertyID == StringID( "m_boxExtents" ) )
            {
                if ( m_pTypeInstance->m_type != RagdollShapeDefinition::Type::Box )
                {
                    return HiddenState::Hidden;
                }
            }

            return HiddenState::Unhandled;
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( RagdollShapeDefinition, RagdollShapeDefinitionEditingRules );
}