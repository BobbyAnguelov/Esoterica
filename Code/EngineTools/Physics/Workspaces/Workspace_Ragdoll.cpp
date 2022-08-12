#include "Workspace_Ragdoll.h"
#include "EngineTools/Core/Widgets/InterfaceHelpers.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "Engine/Physics/Systems/WorldSystem_Physics.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/AnimationBlender.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Animation/AnimationPose.h"
#include "System/Math/MathStringHelpers.h"
#include "System/Math/MathHelpers.h"
#include "Engine/Physics/PhysicsScene.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class [[nodiscard]] ScopedRagdollSettingsModification : public ScopedDescriptorModification
    {
    public:

        ScopedRagdollSettingsModification( RagdollWorkspace* pWorkspace )
            : ScopedDescriptorModification( pWorkspace )
            , m_pRagdollWorkspace( pWorkspace )
        {}

        virtual ~ScopedRagdollSettingsModification()
        {
            if ( m_pRagdollWorkspace->IsPreviewing() )
            {
                m_pRagdollWorkspace->m_refreshRagdollSettings = true;
            }
        }

    private:

        RagdollWorkspace*  m_pRagdollWorkspace = nullptr;
    };
}

//-------------------------------------------------------------------------
// Definition Modification Functions
//-------------------------------------------------------------------------

namespace EE::Physics
{
    static int32_t GetParentBodyIndex( Animation::Skeleton const* pSkeleton, RagdollDefinition const& definition, int32_t bodyIdx )
    {
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < definition.m_bodies.size() );

        int32_t const boneIdx = pSkeleton->GetBoneIndex( definition.m_bodies[bodyIdx].m_boneID );

        // Traverse the hierarchy to try to find the first parent bone that has a body
        int32_t parentBoneIdx = pSkeleton->GetParentBoneIndex( boneIdx );
        while ( parentBoneIdx != InvalidIndex )
        {
            // Do we have a body for this parent bone?
            StringID const parentBoneID = pSkeleton->GetBoneID( parentBoneIdx );
            for ( int32_t i = 0; i < definition.m_bodies.size(); i++ )
            {
                if ( definition.m_bodies[i].m_boneID == parentBoneID )
                {
                    return i;
                }
            }

            parentBoneIdx = pSkeleton->GetParentBoneIndex( parentBoneIdx );
        }

        return InvalidIndex;
    }

    //-------------------------------------------------------------------------

    void RagdollWorkspace::CreateBody( StringID boneID )
    {
        auto pRagdollDefinition = GetRagdollDefinition();
        EE_ASSERT( pRagdollDefinition->IsValid() && m_pSkeleton.IsLoaded() );

        if ( pRagdollDefinition->GetNumBodies() >= RagdollDefinition::s_maxNumBodies )
        {
            return;
        }

        ScopedRagdollSettingsModification const sdm( this );

        EE_ASSERT( pRagdollDefinition->GetBodyIndexForBoneID( boneID ) == InvalidIndex );

        int32_t const newBodyBoneIdx = m_pSkeleton->GetBoneIndex( boneID );
        int32_t const numBodies = pRagdollDefinition->GetNumBodies();
        int32_t bodyInsertionIdx = 0;
        for ( bodyInsertionIdx = 0; bodyInsertionIdx < numBodies; bodyInsertionIdx++ )
        {
            int32_t const existingBodyBoneIdx = m_pSkeleton->GetBoneIndex( pRagdollDefinition->m_bodies[bodyInsertionIdx].m_boneID );
            if ( existingBodyBoneIdx > newBodyBoneIdx )
            {
                break;
            }
        }

        // Body
        //-------------------------------------------------------------------------

        pRagdollDefinition->m_bodies.insert( pRagdollDefinition->m_bodies.begin() + bodyInsertionIdx, RagdollDefinition::BodyDefinition() );
        auto& newBody = pRagdollDefinition->m_bodies[bodyInsertionIdx];
        newBody.m_boneID = boneID;

        // Calculate initial  body and joint transforms
        Transform boneTransform = m_pSkeleton->GetBoneGlobalTransform( newBodyBoneIdx );
        Transform bodyGlobalTransform = boneTransform;

        int32_t const firstChildIdx = m_pSkeleton->GetFirstChildBoneIndex( newBodyBoneIdx );
        if ( firstChildIdx != InvalidIndex )
        {
            auto childBoneTransform = m_pSkeleton->GetBoneGlobalTransform( firstChildIdx );

            Vector bodyAxisX;
            float length = 0.0f;
            ( childBoneTransform.GetTranslation() - boneTransform.GetTranslation() ).ToDirectionAndLength3( bodyAxisX, length );

            // See if we need to adjust the default radius
            float const halfLength = length / 2;
            if ( newBody.m_radius > halfLength )
            {
                newBody.m_radius = halfLength * 2.0f / 3.0f;
            }

            // Calculate the new half-height for the capsule
            newBody.m_halfHeight = Math::Max( ( length / 2 ) - newBody.m_radius, 0.0f );
            Vector const bodyOffset = bodyAxisX * ( newBody.m_halfHeight + newBody.m_radius );

            // Calculate the global transform for the body
            Quaternion const orientation = Quaternion::FromRotationBetweenNormalizedVectors( Vector::UnitX, bodyAxisX );
            bodyGlobalTransform.SetRotation( orientation );
            bodyGlobalTransform.SetTranslation( boneTransform.GetTranslation() + bodyOffset );

            newBody.m_offsetTransform = bodyGlobalTransform * boneTransform.GetInverse();
        }
        else // Root Body
        {
            newBody.m_offsetTransform = boneTransform.GetInverse();
        }

        // Joint Transform
        newBody.m_jointTransform = Transform( bodyGlobalTransform.GetRotation(), boneTransform.GetTranslation() );

        // Profiles
        //-------------------------------------------------------------------------

        for ( auto& profile : pRagdollDefinition->m_profiles )
        {
            // Mass
            profile.m_bodySettings.insert( profile.m_bodySettings.begin() + bodyInsertionIdx, RagdollBodySettings() );
            float const capsuleVolume = Math::CalculateCapsuleVolume( pRagdollDefinition->m_bodies[bodyInsertionIdx].m_radius, pRagdollDefinition->m_bodies[bodyInsertionIdx].m_halfHeight * 2 );
            profile.m_bodySettings[bodyInsertionIdx].m_mass = capsuleVolume * 985.0f; // human body density

            // Materials
            profile.m_materialSettings.insert( profile.m_materialSettings.begin() + bodyInsertionIdx, RagdollBodyMaterialSettings() ); // human body density

            // Joint Settings
            int32_t const jointInsertionIdx = bodyInsertionIdx - 1;
            if ( jointInsertionIdx >= 0 )
            {
                profile.m_jointSettings.insert( profile.m_jointSettings.begin() + jointInsertionIdx, RagdollJointSettings() );
            }
        }
    }

    void RagdollWorkspace::DestroyBody( int32_t bodyIdx )
    {
        ScopedRagdollSettingsModification const sdm( this );

        auto pRagdollDefinition = GetRagdollDefinition();
        EE_ASSERT( pRagdollDefinition->IsValid() && m_pSkeleton.IsLoaded() );
        EE_ASSERT( bodyIdx >= 0 && bodyIdx < pRagdollDefinition->GetNumBodies() );

        // Body
        //-------------------------------------------------------------------------

        pRagdollDefinition->m_bodies.erase( pRagdollDefinition->m_bodies.begin() + bodyIdx );

        // Profiles
        //-------------------------------------------------------------------------

        for ( auto& profile : pRagdollDefinition->m_profiles )
        {
            profile.m_bodySettings.erase( profile.m_bodySettings.begin() + bodyIdx );
            profile.m_materialSettings.erase( profile.m_materialSettings.begin() + bodyIdx );

            int32_t const jointIdx = bodyIdx - 1;
            if ( jointIdx >= 0 )
            {
                profile.m_jointSettings.erase( profile.m_jointSettings.begin() + jointIdx );
            }
        }
    }

    void RagdollWorkspace::DestroyChildBodies( int32_t destructionRootBodyIdx )
    {
        ScopedRagdollSettingsModification const sdm( this );

        auto pRagdollDefinition = GetRagdollDefinition();
        EE_ASSERT( pRagdollDefinition->IsValid() && m_pSkeleton.IsLoaded() );
        EE_ASSERT( destructionRootBodyIdx >= 0 && destructionRootBodyIdx < pRagdollDefinition->GetNumBodies() );

        int32_t const destructionRootBoneIdx = m_pSkeleton->GetBoneIndex( pRagdollDefinition->m_bodies[destructionRootBodyIdx].m_boneID );

        for ( int32_t bodyIdx = 0; bodyIdx < (int32_t) pRagdollDefinition->m_bodies.size(); bodyIdx++ )
        {
            int32_t const bodyBoneIdx = m_pSkeleton->GetBoneIndex( pRagdollDefinition->m_bodies[bodyIdx].m_boneID );
            if ( m_pSkeleton->IsChildBoneOf( destructionRootBoneIdx, bodyBoneIdx ) )
            {
                DestroyBody( bodyIdx );
                bodyIdx--;
            }
        }
    }

    void RagdollWorkspace::AutogenerateBodiesAndJoints()
    {
        ScopedRagdollSettingsModification const sdm( this );

        auto pRagdollDefinition = GetRagdollDefinition();
        EE_ASSERT( pRagdollDefinition->IsValid() && m_pSkeleton.IsLoaded() );

        // Remove all bodies
        //-------------------------------------------------------------------------

        while ( !pRagdollDefinition->m_bodies.empty() )
        {
            DestroyBody( 0 );
        }

        // Recreate all bodies
        //-------------------------------------------------------------------------
        // Skipping the root bone

        int32_t const numBones = m_pSkeleton->GetNumBones();
        int32_t const numBodiesToCreate = Math::Min( numBones, (int32_t) RagdollDefinition::s_maxNumBodies );
        for ( auto i = 1; i < numBodiesToCreate; i++ )
        {
            CreateBody( m_pSkeleton->GetBoneID( i ) );
        }
    }

    void RagdollWorkspace::AutogenerateBodyWeights()
    {
        ScopedRagdollSettingsModification const sdm( this );

        auto pRagdollDefinition = GetRagdollDefinition();
        RagdollDefinition::Profile* pActiveProfile = pRagdollDefinition->GetProfile( m_activeProfileID );
        EE_ASSERT( pActiveProfile != nullptr );

        int32_t const numBodies = pRagdollDefinition->GetNumBodies();
        for ( int32_t i = 0; i < numBodies; i++ )
        {
            float const capsuleVolume = Math::CalculateCapsuleVolume( pRagdollDefinition->m_bodies[i].m_radius, pRagdollDefinition->m_bodies[i].m_halfHeight * 2 );
            pActiveProfile->m_bodySettings[i].m_mass = capsuleVolume * 985; // human body density
        }
    }

    void RagdollWorkspace::CorrectInvalidSettings()
    {
        auto pRagdollDefinition = GetRagdollDefinition();
        for ( auto& profile : pRagdollDefinition->m_profiles )
        {
            profile.CorrectSettingsToValidRanges();
        }
    }

    StringID RagdollWorkspace::CreateProfile()
    {
        ScopedRagdollSettingsModification const sdm( this );

        auto pRagdollDefinition = GetRagdollDefinition();
        int32_t const numBodies = pRagdollDefinition->GetNumBodies();

        auto& profile = pRagdollDefinition->m_profiles.emplace_back();
        
        InlineString ID( InlineString::CtorSprintf(), "%u", (uint64_t) &profile );
        profile.m_ID = StringID( ID.c_str() );

        profile.m_bodySettings.resize( numBodies );
        profile.m_materialSettings.resize( numBodies );

        if ( numBodies > 1 )
        {
            profile.m_jointSettings.resize( numBodies - 1 );
        }

        return profile.m_ID;
    }

    void RagdollWorkspace::DestroyProfile( StringID profileID )
    {
        ScopedRagdollSettingsModification const sdm( this );

        auto pRagdollDefinition = GetRagdollDefinition();
        for ( auto profileIter = pRagdollDefinition->m_profiles.begin(); profileIter != pRagdollDefinition->m_profiles.end(); ++profileIter )
        {
            if ( profileIter->m_ID == profileID )
            {
                pRagdollDefinition->m_profiles.erase( profileIter );
                break;
            }
        }
    }
}

//-------------------------------------------------------------------------
// Drawing functions
//-------------------------------------------------------------------------

namespace EE::Physics
{
    void DrawTwistLimits( Drawing::DrawContext& ctx, Transform const& jointTransform, Radians limitMin, Radians limitMax, Color color, float lineThickness )
    {
        EE_ASSERT( limitMin < limitMax );

        Vector const axisOfRotation = jointTransform.GetAxisX();
        Vector const zeroRotationVector = jointTransform.GetAxisY();

        constexpr float const limitScale = 0.1f;
        constexpr uint32_t const numVertices = 30;
        TInlineVector<Vector, numVertices> vertices;
        
        Radians const deltaAngle = ( limitMax - limitMin ) / numVertices;
        for ( int32_t i = 0; i < numVertices; i++ )
        {
            auto rotatedVector = Quaternion( axisOfRotation, limitMin + ( deltaAngle * i ) ).RotateVector( zeroRotationVector );
            vertices.push_back( jointTransform.GetTranslation() + ( rotatedVector * limitScale ) );
            ctx.DrawLine( jointTransform.GetTranslation(), vertices.back(), color, lineThickness );
        }

        for ( auto i = 1; i < numVertices; i++ )
        {
            ctx.DrawLine( vertices[i-1], vertices[i], color, lineThickness);
        }
    }

    void DrawSwingLimits( Drawing::DrawContext& ctx, Transform const& jointTransform, Radians limitY, Radians limitZ, Color color, float lineThickness )
    {
        Vector const axisOfRotationY = jointTransform.GetAxisY();
        Vector const axisOfRotationZ = jointTransform.GetAxisZ();
        Vector const zeroRotationVector = jointTransform.GetAxisX();

        constexpr float const limitScale = 0.2f;
        constexpr uint32_t const numVertices = 30;
        TInlineVector<Vector, numVertices> vertices;

        Radians const startAngleY = -( limitY / 2 );
        Radians const deltaAngleY = limitY / numVertices;
        for ( int32_t i = 0; i < numVertices; i++ )
        {
            auto rotatedVector = Quaternion( axisOfRotationY, startAngleY + ( deltaAngleY * i ) ).RotateVector( zeroRotationVector );
            vertices.push_back( jointTransform.GetTranslation() + ( rotatedVector * limitScale ) );
            ctx.DrawLine( jointTransform.GetTranslation(), vertices.back(), Colors::Lime, lineThickness );
        }

        vertices.clear();

        Radians const startAngleZ = -( limitZ / 2 );
        Radians const deltaAngleZ = limitZ / numVertices;
        for ( int32_t i = 0; i < numVertices; i++ )
        {
            auto rotatedVector = Quaternion( axisOfRotationZ, startAngleZ + ( deltaAngleZ * i ) ).RotateVector( zeroRotationVector );
            vertices.push_back( jointTransform.GetTranslation() + ( rotatedVector * limitScale ) );
            ctx.DrawLine( jointTransform.GetTranslation(), vertices.back(), Colors::RoyalBlue, lineThickness );
        }
    }
}

//-------------------------------------------------------------------------
// Workspace Functions
//-------------------------------------------------------------------------

namespace EE::Physics
{
    EE_RESOURCE_WORKSPACE_FACTORY( RagdollWorkspaceFactory, RagdollDefinition, RagdollWorkspace );

    //-------------------------------------------------------------------------

    RagdollWorkspace::RagdollWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID, bool shouldLoadResource )
        : TResourceWorkspace<RagdollDefinition>( pToolsContext, pWorld, resourceID, shouldLoadResource )
        , m_bodyEditorPropertyGrid( pToolsContext )
        , m_solverSettingsGrid( pToolsContext )
        , m_resourceFilePicker( *pToolsContext )
    {
        auto OnPreEdit = [this] ( PropertyEditInfo const& info ) 
        {
            PreEdit( info );
        };
        
        auto OnPostEdit = [this] ( PropertyEditInfo const& info ) 
        {
            PostEdit( info );
        };

        m_bodyGridPreEditEventBindingID = m_bodyEditorPropertyGrid.OnPreEdit().Bind( OnPreEdit );
        m_bodyGridPostEditEventBindingID = m_bodyEditorPropertyGrid.OnPostEdit().Bind( OnPostEdit );

        //-------------------------------------------------------------------------

        auto OnSolverPreEdit = [this] ( PropertyEditInfo const& info )
        {
            PreEdit( info );
        };

        auto OnSolverPostEdit = [this] ( PropertyEditInfo const& info )
        {
            auto pSolverSettings = Cast<RagdollDefinition::Profile>( m_solverSettingsGrid.GetEditedType() );
            pSolverSettings->CorrectSettingsToValidRanges();
            PostEdit( info );
        };

        m_solverGridPreEditEventBindingID = m_solverSettingsGrid.OnPreEdit().Bind( OnSolverPreEdit );
        m_solverGridPostEditEventBindingID = m_solverSettingsGrid.OnPostEdit().Bind( OnSolverPostEdit );

        //-------------------------------------------------------------------------

        m_gizmo.SetOption( ImGuiX::Gizmo::Options::DrawManipulationPlanes, false );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowCoordinateSpaceSwitching, false );
        m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Translation );
        m_gizmo.SetTargetTransform( &m_gizmoWorkingTransform );

        m_previewGizmo.SetOption( ImGuiX::Gizmo::Options::DrawManipulationPlanes, false );
        m_previewGizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );
        m_previewGizmo.SetOption( ImGuiX::Gizmo::Options::AllowCoordinateSpaceSwitching, false );
        m_previewGizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Translation );
        m_previewGizmo.SetTargetTransform( &m_previewGizmoTransform );
    }

    RagdollWorkspace::~RagdollWorkspace()
    {
        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
        EE_ASSERT( !m_pSkeleton.IsValid() || m_pSkeleton.IsUnloaded() );

        m_bodyEditorPropertyGrid.OnPreEdit().Unbind( m_bodyGridPreEditEventBindingID );
        m_bodyEditorPropertyGrid.OnPostEdit().Unbind( m_bodyGridPostEditEventBindingID );
        m_solverSettingsGrid.OnPreEdit().Unbind( m_solverGridPreEditEventBindingID );
        m_solverSettingsGrid.OnPostEdit().Unbind( m_solverGridPostEditEventBindingID );
    }

    void RagdollWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID viewportDockID = 0;
        ImGuiID leftDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.3f, nullptr, &viewportDockID );
        ImGuiID bottomleftDockID = ImGui::DockBuilderSplitNode( leftDockID, ImGuiDir_Down, 0.3f, nullptr, &leftDockID );
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Down, 0.2f, nullptr, &viewportDockID );
        ImGuiID rightDockID = ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Right, 0.2f, nullptr, &viewportDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetViewportWindowID(), viewportDockID );
        ImGui::DockBuilderDockWindow( m_bodyEditorWindowName.c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( m_bodyEditorDetailsWindowName.c_str(), bottomleftDockID );
        ImGui::DockBuilderDockWindow( m_profileEditorWindowName.c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( m_descriptorWindowName.c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( m_previewControlsWindowName.c_str(), rightDockID );
    }

    void RagdollWorkspace::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        TResourceWorkspace<RagdollDefinition>::PostUndoRedo( operation, pAction );
        SetActiveProfile( m_activeProfileID );
    }

    //-------------------------------------------------------------------------

    void RagdollWorkspace::Initialize( UpdateContext const& context )
    {
        TResourceWorkspace<RagdollDefinition>::Initialize( context );
        m_bodyEditorWindowName.sprintf( "Body Editor##%u", GetID() );
        m_bodyEditorDetailsWindowName.sprintf( "Details##%u", GetID() );
        m_profileEditorWindowName.sprintf( "Profile Editor##%u", GetID() );
        m_previewControlsWindowName.sprintf( "Preview Controls##%u", GetID() );
        LoadDefinition();
    }

    void RagdollWorkspace::Shutdown( UpdateContext const& context )
    {
        // Stop preview
        if ( IsPreviewing() )
        {
            StopPreview();
        }

        // Unload preview animation
        if ( m_pPreviewAnimation.IsValid() && !m_pPreviewAnimation.IsUnloaded() )
        {
            UnloadResource( &m_pPreviewAnimation );
        }

        //-------------------------------------------------------------------------

        UnloadDefinition();
        DestroySkeletonTree();
        TResourceWorkspace<RagdollDefinition>::Shutdown( context );
    }

    void RagdollWorkspace::LoadDefinition()
    {
        auto pRagdollDefinition = GetRagdollDefinition();
        if ( pRagdollDefinition->IsValid() )
        {
            for ( auto& profile : pRagdollDefinition->m_profiles )
            {
                if ( profile.CorrectSettingsToValidRanges() )
                {
                    MarkDirty();
                }
            }

            //-------------------------------------------------------------------------

            m_pSkeleton = GetRagdollDefinition()->m_pSkeleton;
            LoadResource( &m_pSkeleton );

            // Load resource descriptor for skeleton to get the preview mesh
            auto resourceDescPath = GetFileSystemPath( m_pSkeleton.GetResourcePath() );
            Animation::SkeletonResourceDescriptor skeletonResourceDesc;
            bool const result = Resource::ResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, resourceDescPath, skeletonResourceDesc );
            EE_ASSERT( result );

            // Create preview entity
            ResourceID const previewMeshResourceID( skeletonResourceDesc.m_previewMesh.GetResourcePath() );
            if ( previewMeshResourceID.IsValid() )
            {
                m_pMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
                m_pMeshComponent->SetSkeleton( m_pSkeleton.GetResourceID() );
                m_pMeshComponent->SetMesh( previewMeshResourceID );

                m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
                m_pPreviewEntity->AddComponent( m_pMeshComponent );
                AddEntityToWorld( m_pPreviewEntity );
            }

            // Update active profile
            if ( pRagdollDefinition->GetProfile( m_activeProfileID ) == nullptr )
            {
                SetActiveProfile( pRagdollDefinition->GetDefaultProfile()->m_ID );
            }

            m_definitionLoaded = true;
        }
    }

    void RagdollWorkspace::UnloadDefinition()
    {
        if ( m_pPreviewEntity != nullptr )
        {
            DestroyEntityInWorld( m_pPreviewEntity );
            m_pMeshComponent = nullptr;
        }

        if ( m_pSkeleton.IsValid() && !m_pSkeleton.IsUnloaded() )
        {
            UnloadResource( &m_pSkeleton );
        }

        m_definitionLoaded = false;
    }

    void RagdollWorkspace::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        if ( IsPreviewing() )
        {
            StopPreview();
        }

        //-------------------------------------------------------------------------

        TResourceWorkspace<RagdollDefinition>::BeginHotReload( usersToBeReloaded, resourcesToBeReloaded );

        // If the descriptor is being reloaded
        if ( m_pDescriptor == nullptr )
        {
            UnloadDefinition();
        }
    }

    void RagdollWorkspace::EndHotReload()
    {
        TResourceWorkspace<RagdollDefinition>::EndHotReload();

        if ( !m_definitionLoaded && m_pDescriptor != nullptr )
        {
            LoadDefinition();
        }
    }

    //-------------------------------------------------------------------------

    void RagdollWorkspace::DrawWorkspaceToolbarItems( UpdateContext const& context )
    {
        ImGuiX::VerticalSeparator( ImVec2( 15, -1 ) );

        auto pRagdollDefinition = GetRagdollDefinition();

        //-------------------------------------------------------------------------

        ImVec2 const menuDimensions = ImGui::GetContentRegionMax();
        float buttonDimensions = 130;
        ImGui::SameLine( menuDimensions.x / 2 - buttonDimensions / 2 );

        if ( IsPreviewing() )
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_STOP, "Stop Preview", Colors::Red, ImVec2( buttonDimensions, 0 ) ) )
            {
                StopPreview();
            }
        }
        else
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_PLAY, "Preview Ragdoll", Colors::Lime, ImVec2( buttonDimensions, 0 ) ) )
            {
                StartPreview( context );
            }
            ImGui::EndDisabled();
        }

        // Gap
        //-------------------------------------------------------------------------

        float const availableX = ImGui::GetContentRegionAvail().x;
        if ( availableX > 280 )
        {
            ImGui::Dummy( ImVec2( availableX - 280, 0 ) );
        }
        else
        {
            ImGuiX::VerticalSeparator();
        }

        // Editing Options
        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( IsPreviewing() );
        if ( ImGui::BeginMenu( EE_ICON_ACCOUNT_WRENCH" Editing Options" ) )
        {
            ImGuiX::TextSeparator( "Visualization" );

            ImGui::MenuItem( "Show Skeleton", nullptr, &m_drawSkeleton );

            ImGui::MenuItem( "Show Body Axes", nullptr, &m_drawBodyAxes );

            ImGui::MenuItem( "Show Only Selected Bodies", nullptr, &m_isolateSelectedBody );

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Authoring" );

            if ( ImGui::MenuItem( "Autogenerate Bodies + joints" ) )
            {
                AutogenerateBodiesAndJoints();
            }

            if ( ImGui::MenuItem( "Autogenerate Body Masses" ) )
            {
                AutogenerateBodyWeights();
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Fix" );

            if ( ImGui::MenuItem( "Fix invalid settings" ) )
            {
                CorrectInvalidSettings();
            }

            ImGui::EndMenu();
        }
        ImGui::EndDisabled();

        // Preview Options
        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( EE_ICON_TELEVISION_PLAY" Preview Options" ) )
        {
            ImGui::MenuItem( "Draw Ragdoll", nullptr, &m_drawRagdoll );
            ImGui::MenuItem( "Draw Anim Pose", nullptr, &m_drawAnimationPose );
            ImGui::EndMenu();
        }
    }

    void RagdollWorkspace::DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        TResourceWorkspace<RagdollDefinition>::DrawViewportOverlayElements( context, pViewport );

        auto pRagdollDefinition = GetRagdollDefinition();
        if ( !pRagdollDefinition->IsValid() )
        {
            return;
        }

        if ( !m_pSkeleton.IsLoaded() )
        {
            return;
        }

        if ( IsPreviewing() )
        {
            m_previewGizmoTransform = m_pMeshComponent->GetWorldTransform();
            if ( m_previewGizmo.Draw( *pViewport ) != ImGuiX::Gizmo::Result::NoResult )
            {
                m_pMeshComponent->SetWorldTransform( m_previewGizmoTransform );
            }
        }
        else
        {
            auto drawingCtx = GetDrawingContext();

            // Draw skeleton
            if ( m_drawSkeleton )
            {
                drawingCtx.Draw( *m_pSkeleton.GetPtr(), Transform::Identity );
            }

            //-------------------------------------------------------------------------

            if ( m_editorMode == Mode::BodyEditor )
            {
                int32_t const numBodies = pRagdollDefinition->GetNumBodies();
                for ( auto i = 0; i < numBodies; i++ )
                {
                    auto const& body = pRagdollDefinition->m_bodies[i];
                    int32_t const boneIdx = m_pSkeleton->GetBoneIndex( body.m_boneID );
                    EE_ASSERT( boneIdx != InvalidIndex );

                    if ( m_isolateSelectedBody && body.m_boneID != m_selectedBoneID )
                    {
                        continue;
                    }

                    //-------------------------------------------------------------------------

                    Transform const bodyTransform = body.m_offsetTransform * m_pSkeleton->GetBoneGlobalTransform( boneIdx );
                    drawingCtx.DrawCapsuleHeightX( bodyTransform, body.m_radius, body.m_halfHeight, Colors::Orange, 3.0f );

                    // Draw body axes
                    if ( m_drawBodyAxes )
                    {
                        drawingCtx.DrawAxis( bodyTransform, 0.025f, 2 );
                    }

                    // Draw joint
                    if ( i > 0 )
                    {
                        int32_t const parentBodyIdx = GetParentBodyIndex( m_pSkeleton.GetPtr(), *pRagdollDefinition, i );
                        int32_t const parentBoneIdx = m_pSkeleton->GetBoneIndex( pRagdollDefinition->m_bodies[parentBodyIdx].m_boneID );
                        EE_ASSERT( parentBoneIdx != InvalidIndex );
                        Transform const parentBodyTransform = pRagdollDefinition->m_bodies[parentBodyIdx].m_offsetTransform * m_pSkeleton->GetBoneGlobalTransform( parentBoneIdx );

                        drawingCtx.DrawAxis( body.m_jointTransform, 0.1f, 3 );
                        drawingCtx.DrawLine( body.m_jointTransform.GetTranslation(), parentBodyTransform.GetTranslation(), Colors::Cyan, 2.0f );
                        drawingCtx.DrawLine( body.m_jointTransform.GetTranslation(), bodyTransform.GetTranslation(), Colors::Cyan, 2.0f );
                    }
                }
            }
            else
            {
                int32_t const numBodies = pRagdollDefinition->GetNumBodies();
                auto pActiveProfile = pRagdollDefinition->GetProfile( m_activeProfileID );

                for ( auto bodyIdx = 1; bodyIdx < numBodies; bodyIdx++ )
                {
                    int32_t const jointIdx = bodyIdx - 1;
                    auto const& body = pRagdollDefinition->m_bodies[bodyIdx];
                    int32_t const boneIdx = m_pSkeleton->GetBoneIndex( body.m_boneID );
                    EE_ASSERT( boneIdx != InvalidIndex );

                    if ( m_isolateSelectedBody && body.m_boneID != m_selectedBoneID )
                    {
                        continue;
                    }

                    int32_t const parentBodyIdx = GetParentBodyIndex( m_pSkeleton.GetPtr(), *pRagdollDefinition, bodyIdx );
                    int32_t const parentBoneIdx = m_pSkeleton->GetBoneIndex( pRagdollDefinition->m_bodies[parentBodyIdx].m_boneID );
                    EE_ASSERT( parentBoneIdx != InvalidIndex );

                    Transform const bodyTransform = body.m_offsetTransform * m_pSkeleton->GetBoneGlobalTransform( boneIdx );
                    Transform const parentBodyTransform = pRagdollDefinition->m_bodies[parentBodyIdx].m_offsetTransform * m_pSkeleton->GetBoneGlobalTransform( parentBoneIdx );

                    // Draw joint
                    drawingCtx.DrawAxis( body.m_jointTransform, 0.1f, 4 );
                    drawingCtx.DrawLine( body.m_jointTransform.GetTranslation(), parentBodyTransform.GetTranslation(), Colors::Cyan, 2.0f );
                    drawingCtx.DrawLine( body.m_jointTransform.GetTranslation(), bodyTransform.GetTranslation(), Colors::Cyan, 2.0f );

                    // Draw limits
                    if ( pActiveProfile->m_jointSettings[jointIdx].m_twistLimitEnabled )
                    {
                        DrawTwistLimits( drawingCtx, body.m_jointTransform, Degrees( pActiveProfile->m_jointSettings[jointIdx].m_twistLimitMin ), Degrees( pActiveProfile->m_jointSettings[jointIdx].m_twistLimitMax ), Colors::Red, 1.0f );
                    }

                    if ( pActiveProfile->m_jointSettings[jointIdx].m_swingLimitEnabled )
                    {
                        DrawSwingLimits( drawingCtx, body.m_jointTransform, Degrees( pActiveProfile->m_jointSettings[jointIdx].m_swingLimitY ), Degrees( pActiveProfile->m_jointSettings[jointIdx].m_swingLimitZ ), Colors::Yellow, 1.0f );
                    }
                }
            }

            // Gizmo
            //-------------------------------------------------------------------------

            if ( m_selectedBoneID.IsValid() )
            {
                int32_t const boneIdx = m_pSkeleton->GetBoneIndex( m_selectedBoneID );
                int32_t const bodyIdx = pRagdollDefinition->GetBodyIndexForBoneID( m_selectedBoneID );
                if ( bodyIdx != InvalidIndex )
                {
                    auto& body = pRagdollDefinition->m_bodies[bodyIdx];

                    if ( m_editorMode == Mode::BodyEditor )
                    {
                        Transform const boneTransform = m_pSkeleton->GetBoneGlobalTransform( boneIdx );
                        Transform const bodyTransform = body.m_offsetTransform * boneTransform;
                        m_gizmoWorkingTransform = bodyTransform;
                 
                        switch ( m_gizmo.Draw( *pViewport ) )
                        {
                            case ImGuiX::Gizmo::Result::StartedManipulating :
                            {
                                BeginModification();
                                body.m_offsetTransform = m_gizmoWorkingTransform * boneTransform.GetInverse();
                            }
                            break;

                            case ImGuiX::Gizmo::Result::Manipulating :
                            {
                                body.m_offsetTransform = m_gizmoWorkingTransform * boneTransform.GetInverse();
                            }
                            break;

                            case ImGuiX::Gizmo::Result::StoppedManipulating :
                            {
                                EndModification();
                            }
                            break;
                        }
                    }
                    else
                    {
                        m_gizmoWorkingTransform = body.m_jointTransform;

                        switch ( m_gizmo.Draw( *pViewport ) )
                        {
                            case ImGuiX::Gizmo::Result::StartedManipulating:
                            {
                                BeginModification();
                                body.m_jointTransform = m_gizmoWorkingTransform;
                            }
                            break;

                            case ImGuiX::Gizmo::Result::Manipulating:
                            {
                                body.m_jointTransform = m_gizmoWorkingTransform;
                            }
                            break;

                            case ImGuiX::Gizmo::Result::StoppedManipulating:
                            {
                                EndModification();
                            }
                            break;
                        }
                    }
                }

                if ( m_isViewportFocused )
                {
                    if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
                    {
                        m_gizmo.SwitchToNextMode();
                    }
                }
            }
        }
    }

    void RagdollWorkspace::UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        // Track ragdoll definition state changes
        //-------------------------------------------------------------------------

        auto pRagdollDefinition = GetRagdollDefinition();
        
        bool const skeletonsMatch = m_pSkeleton.GetResourceID() == pRagdollDefinition->m_pSkeleton.GetResourceID();
        if ( !skeletonsMatch && m_definitionLoaded )
        {
            UnloadDefinition();
        }

        if ( IsPreviewing() && !pRagdollDefinition->IsValid() )
        {
            StopPreview();
        }

        //-------------------------------------------------------------------------

        if ( m_definitionLoaded )
        {
            if ( !pRagdollDefinition->IsValid() )
            {
                UnloadDefinition();
            }
        }
        else
        {
            if ( pRagdollDefinition->IsValid() )
            {
                LoadDefinition();
            }
        }

        // Editor UI
        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( IsPreviewing() );
        {
            ImGui::SetNextWindowClass( pWindowClass );
            DrawBodyEditorWindow( context );

            ImGui::SetNextWindowClass( pWindowClass );
            DrawBodyEditorDetailsWindow( context );

            DrawDescriptorEditorWindow( context, pWindowClass );
        }
        ImGui::EndDisabled();

        ImGui::SetNextWindowClass( pWindowClass );
        DrawProfileEditorWindow( context );

        // Preview UI
        //-------------------------------------------------------------------------

        ImGui::SetNextWindowClass( pWindowClass );
        DrawPreviewControlsWindow( context );
    }

    //-------------------------------------------------------------------------

    void RagdollWorkspace::DrawBodyEditorWindow( UpdateContext const& context )
    {
        if ( ImGui::Begin( m_bodyEditorWindowName.c_str() ) )
        {
            if ( m_pSkeleton.IsLoaded() )
            {
                if ( m_pSkeletonTreeRoot == nullptr )
                {
                    CreateSkeletonTree();
                }

                ImGui::Text( "Skeleton ID: %s", m_pSkeleton->GetResourcePath().GetFileNameWithoutExtension().c_str());

                if ( m_editorMode == Mode::BodyEditor )
                {
                    if( ImGuiX::ColoredButton( Colors::RoyalBlue, Colors::White, "Switch to Joint Editor", ImVec2( -1, 0 ) ) )
                    {
                        m_editorMode = Mode::JointEditor;
                    }
                }
                else
                {
                    if ( ImGuiX::ColoredButton( Colors::Orange, Colors::Black, "Switch to Body Editor", ImVec2( -1, 0 ) ) )
                    {
                        m_editorMode = Mode::BodyEditor;
                    }
                }

                ImGui::Separator();

                EE_ASSERT( m_pSkeletonTreeRoot != nullptr );
                RenderSkeletonTree( m_pSkeletonTreeRoot );
            }
        }
        ImGui::End();
    }

    void RagdollWorkspace::DrawBodyEditorDetailsWindow( UpdateContext const& context )
    {
        auto pRagdollDefinition = GetRagdollDefinition();
        if ( m_selectedBoneID.IsValid() )
        {
            int32_t const bodyIdx = pRagdollDefinition->GetBodyIndexForBoneID( m_selectedBoneID );
            if ( bodyIdx != InvalidIndex )
            {
                m_bodyEditorPropertyGrid.SetTypeToEdit( &pRagdollDefinition->m_bodies[bodyIdx] );
            }
            else
            {
                m_bodyEditorPropertyGrid.SetTypeToEdit( nullptr );
            }
        }
        else
        {
            m_bodyEditorPropertyGrid.SetTypeToEdit( nullptr );
        }

        //-------------------------------------------------------------------------

        if ( ImGui::Begin( m_bodyEditorDetailsWindowName.c_str() ) )
        {
            m_bodyEditorPropertyGrid.DrawGrid();
        }
        ImGui::End();
    }

    void RagdollWorkspace::CreateSkeletonTree()
    {
        TVector<BoneInfo*> boneInfos;

        // Create all infos
        int32_t const numBones = m_pSkeleton->GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& pBoneInfo = boneInfos.emplace_back( EE::New<BoneInfo>() );
            pBoneInfo->m_boneIdx = i;
        }

        // Create hierarchy
        for ( auto i = 1; i < numBones; i++ )
        {
            int32_t const parentBoneIdx = m_pSkeleton->GetParentBoneIndex( i );
            EE_ASSERT( parentBoneIdx != InvalidIndex );
            boneInfos[parentBoneIdx]->m_children.emplace_back( boneInfos[i] );
        }

        // Set root
        m_pSkeletonTreeRoot = boneInfos[0];
    }

    void RagdollWorkspace::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    ImRect RagdollWorkspace::RenderSkeletonTree( BoneInfo* pBone )
    {
        EE_ASSERT( pBone != nullptr );
        auto pRagdollDefinition = GetRagdollDefinition();

        //-------------------------------------------------------------------------

        StringID const currentBoneID = m_pSkeleton->GetBoneID( pBone->m_boneIdx );
        int32_t const bodyIdx = pRagdollDefinition->GetBodyIndexForBoneID( currentBoneID );
        bool const hasBody = bodyIdx != InvalidIndex;

        //-------------------------------------------------------------------------

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

        if ( hasBody )
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::MediumBold, Colors::LimeGreen );
            pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
        }
        else
        {
            pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
        }

        // Handle bone selection
        if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenBlockedByPopup ) )
        {
            if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
            {
                m_selectedBoneID = m_pSkeleton->GetBoneID( pBone->m_boneIdx );
            }
        }

        // Context Menu
        if ( ImGui::BeginPopupContextItem() )
        {
            if ( hasBody )
            {
                if ( ImGui::MenuItem( "Destroy Body" ) )
                {
                    DestroyBody( bodyIdx );
                    SetActiveProfile( m_activeProfileID );
                }

                if ( ImGui::MenuItem( "Destroy All Child Bodies" ) )
                {
                    DestroyChildBodies( bodyIdx );
                    SetActiveProfile( m_activeProfileID );
                }
            }
            else
            {
                if ( pRagdollDefinition->GetNumBodies() >= RagdollDefinition::s_maxNumBodies )
                {
                    ImGui::Text( "Max number of bodies reached" );
                }
                else
                {
                    if ( ImGui::MenuItem( "Create Body" ) )
                    {
                        CreateBody( currentBoneID );
                        SetActiveProfile( m_activeProfileID );
                    }
                }
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
                const ImRect childRect = RenderSkeletonTree( pChild );
                const float midpoint = ( childRect.Min.y + childRect.Max.y ) / 2.0f;
                drawList->AddLine( ImVec2( verticalLineStart.x, midpoint ), ImVec2( verticalLineStart.x + HorizontalTreeLineSize, midpoint ), TreeLineColor );
                verticalLineEnd.y = midpoint;
            }

            drawList->AddLine( verticalLineStart, verticalLineEnd, TreeLineColor );
            ImGui::TreePop();
        }

        return nodeRect;
    }

    //-------------------------------------------------------------------------

    void RagdollWorkspace::SetActiveProfile( StringID profileID )
    {
        auto pRagdollDefinition = GetRagdollDefinition();
        m_activeProfileID = profileID;
        m_workingProfileCopy = *pRagdollDefinition->GetProfile( profileID );

        if ( IsPreviewing() )
        {
            m_pRagdoll->SwitchProfile( m_activeProfileID );
        }
    }

    void RagdollWorkspace::DrawProfileEditorWindow( UpdateContext const& context )
    {
        auto pRagdollDefinition = GetRagdollDefinition();

        if ( ImGui::Begin( m_profileEditorWindowName.c_str() ) )
        {
            DrawProfileManager();

            //-------------------------------------------------------------------------

            RagdollDefinition::Profile* pActiveProfile = nullptr;
            if ( pRagdollDefinition->IsValid() )
            {
                pActiveProfile = pRagdollDefinition->GetProfile( m_activeProfileID );
            }

            //-------------------------------------------------------------------------

            if ( pActiveProfile == nullptr )
            {
                ImGui::TextColored( Colors::Red.ToFloat4(), "Invalid Definition" );
                m_solverSettingsGrid.SetTypeToEdit( nullptr );
            }
            else if ( pRagdollDefinition->GetNumBodies() == 0 )
            {
                ImGui::TextColored( Colors::Yellow.ToFloat4(), "No Bodies" );
                m_solverSettingsGrid.SetTypeToEdit( nullptr );
            }
            else
            {
                if ( ImGui::BeginTabBar( "ProfileSetings" ) )
                {
                    if ( ImGui::BeginTabItem( "Body/Joint Settings" ) )
                    {
                        DrawRootBodySettings( context, pActiveProfile );
                        DrawJointSettingsTable( context, pActiveProfile );
                        ImGui::EndTabItem();
                    }

                    //-------------------------------------------------------------------------

                    if ( ImGui::BeginTabItem( "Material Settings" ) )
                    {
                        DrawMaterialSettingsTable( context, pActiveProfile );
                        ImGui::EndTabItem();
                    }

                    //-------------------------------------------------------------------------

                    if ( ImGui::BeginTabItem( "Solver Settings" ) )
                    {
                        if ( m_solverSettingsGrid.GetEditedType() != pActiveProfile )
                        {
                            m_solverSettingsGrid.SetTypeToEdit( pActiveProfile );
                        }
                        m_solverSettingsGrid.DrawGrid();
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
            }
        }
        ImGui::End();
    }

    void RagdollWorkspace::DrawProfileManager()
    {
        constexpr static float const renameButtonWidth = 24;
        constexpr static float const buttonWidth = 100;
        float const spacing = ImGui::GetStyle().ItemSpacing.x;

        auto pRagdollDefinition = GetRagdollDefinition();
        if ( !pRagdollDefinition->IsValid() )
        {
            return;
        }

        // Profile Selector
        //-------------------------------------------------------------------------

        ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - ( renameButtonWidth + spacing ) - ( 2 * ( buttonWidth + spacing ) ) );
        if ( ImGui::BeginCombo( "##ProfileSelector", m_activeProfileID.IsValid() ? m_activeProfileID.c_str() : "Invalid Profile ID" ) )
        {
            for ( auto const& profile : pRagdollDefinition->m_profiles )
            {
                bool const isSelected = ( profile.m_ID == m_activeProfileID );
                if ( ImGui::Selectable( profile.m_ID.c_str(), isSelected ) )
                {
                    SetActiveProfile( profile.m_ID );
                }

                if ( isSelected )
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        // Controls
        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( IsPreviewing() );

        ImGui::SameLine();
        if ( ImGuiX::ColoredButton( Colors::RoyalBlue, Colors::White, EE_ICON_FORM_TEXTBOX"##Rename", ImVec2( renameButtonWidth, 0 ) ) )
        {
            Printf( m_profileNameBuffer, 256, m_activeProfileID.c_str() );
            m_originalPrenameID = m_activeProfileID;
            ImGui::OpenPopup( "Rename Profile" );
        }

        ImGui::SameLine();
        if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, "New Profile", ImVec2( buttonWidth, 0 ) ) )
        {
            SetActiveProfile( CreateProfile() );
            Printf( m_profileNameBuffer, 256, m_activeProfileID.c_str() );
            m_originalPrenameID = m_activeProfileID;
            ImGui::OpenPopup( "Rename Profile" );
        }

        ImGui::SameLine();
        ImGui::BeginDisabled( pRagdollDefinition->GetNumProfiles() == 1 );
        if ( ImGuiX::ColoredButton( Colors::MediumRed, Colors::White, "Delete", ImVec2( buttonWidth, 0 ) ) )
        {
            if ( pRagdollDefinition->GetNumProfiles() > 1 )
            {
                DestroyProfile( m_activeProfileID );
                SetActiveProfile( pRagdollDefinition->GetDefaultProfile()->m_ID );
            }
        }

        ImGui::EndDisabled();

        // Rename Dialog
        //-------------------------------------------------------------------------

        ImGui::SetNextWindowSize( ImVec2( 600, 52 ) );
        if ( ImGui::BeginPopupModal( "Rename Profile", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize ) )
        {
            auto ValidateProfileName = [this, pRagdollDefinition] ( char const* const pName )
            {
                StringID potentialID( pName );
                if ( !potentialID.IsValid() )
                {
                    return false;
                }

                if ( potentialID == m_originalPrenameID )
                {
                    return true;
                }

                return pRagdollDefinition->GetProfile( potentialID ) == nullptr;
            };

            auto pProfile = pRagdollDefinition->GetProfile( m_activeProfileID );
            if ( pProfile != nullptr )
            {
                auto CommitChanges = [this, pProfile, ValidateProfileName] ()
                {
                    if ( ValidateProfileName( m_profileNameBuffer ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        pProfile->m_ID = StringID( m_profileNameBuffer );
                        SetActiveProfile( pProfile->m_ID );
                        ImGui::CloseCurrentPopup();
                    }
                };

                ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - 40 - ImGui::GetStyle().ItemSpacing.x );
                if ( ImGui::InputText( "##ProfileName", m_profileNameBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue ) )
                {
                    CommitChanges();
                }

                ImGui::SameLine();
                ImGui::BeginDisabled( !ValidateProfileName( m_profileNameBuffer ) );
                if ( ImGui::Button( "OK", ImVec2( 40, 0 ) ) )
                {
                    CommitChanges();
                }
                ImGui::EndDisabled();
            }
            else
            {
                ImGui::CloseCurrentPopup();
                SetActiveProfile( pRagdollDefinition->GetDefaultProfile()->m_ID );
            }

            ImGui::EndPopup();
        }

        ImGui::EndDisabled();
    }

    void RagdollWorkspace::DrawRootBodySettings( UpdateContext const& context, RagdollDefinition::Profile* pProfile )
    {
        static char const* const rootBodyDriveComboOptions[3] = { "None", "Distance", "Spring" };

        EE_ASSERT( pProfile != nullptr );
        auto pRagdollDefinition = GetRagdollDefinition();

        ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
        if ( ImGui::BeginTable( "BodySettingsTable", 9, ImGuiTableFlags_BordersInnerV ) )
        {
            ImGui::TableSetupColumn( "Root Body", ImGuiTableColumnFlags_WidthFixed, 120 );
            ImGui::TableSetupColumn( "Mass", ImGuiTableColumnFlags_WidthFixed, 40 );
            ImGui::TableSetupColumn( "CCD", ImGuiTableColumnFlags_WidthFixed, 20 );
            ImGui::TableSetupColumn( "Self Collision", ImGuiTableColumnFlags_WidthFixed, 30 );

            ImGui::TableSetupColumn( "Drive", ImGuiTableColumnFlags_WidthFixed, 75 );

            ImGui::TableSetupColumn( "Stiffness", ImGuiTableColumnFlags_WidthFixed, 70 );
            ImGui::TableSetupColumn( "Damping", ImGuiTableColumnFlags_WidthFixed, 70 );

            ImGui::TableSetupColumn( "Max Distance", ImGuiTableColumnFlags_WidthFixed, 70 );
            ImGui::TableSetupColumn( "Tolerance", ImGuiTableColumnFlags_WidthFixed, 70 );

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            auto& cachedBodySettings = m_workingProfileCopy.m_bodySettings[0];
            auto& realBodySettings = pProfile->m_bodySettings[0];

            auto& cachedSettings = m_workingProfileCopy.m_rootControlBodySettings;
            auto& realSettings = pProfile->m_rootControlBodySettings;

            //-------------------------------------------------------------------------

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text( pRagdollDefinition->m_bodies[0].m_boneID.c_str() );

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth( -1 );
            if ( ImGui::InputFloat( "##Mass", &cachedBodySettings.m_mass, 0.0f, 0.0f, "%.2f" ) )
            {
                cachedBodySettings.m_mass = Math::Clamp( cachedBodySettings.m_mass, 0.0f, 1000000.0f );
            }
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ScopedRagdollSettingsModification const sdm( this );
                realBodySettings.m_mass = cachedBodySettings.m_mass;
            }
            ImGuiX::ItemTooltip( "The mass of the body" );

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::Checkbox( "##CCD", &cachedBodySettings.m_enableCCD );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ScopedRagdollSettingsModification const sdm( this );
                realBodySettings.m_enableCCD = cachedBodySettings.m_enableCCD;
            }
            ImGuiX::ItemTooltip( "Enable continuous collision detection for this body" );

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::Checkbox( "##SC", &cachedBodySettings.m_enableSelfCollision );
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ScopedRagdollSettingsModification const sdm( this );
                realBodySettings.m_enableSelfCollision = cachedBodySettings.m_enableSelfCollision;
            }
            ImGuiX::ItemTooltip( "Should this body collide with other bodies in the ragdoll" );

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth( -1 );

            int32_t dt = (int32_t) cachedSettings.m_driveType;
            if ( ImGui::Combo( "##DriveMode", &dt, rootBodyDriveComboOptions, 3 ) )
            {
                ScopedRagdollSettingsModification const sdm( this );
                cachedSettings.m_driveType = (RagdollRootControlBodySettings::DriveType) dt;
                realSettings.m_driveType = cachedSettings.m_driveType;

                if ( IsPreviewing() )
                {
                    m_pRagdoll->RecreateRootControlBody();
                }
            }

            ImGuiX::ItemTooltip( "The drive mode for the root control body's joint" );

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( realSettings.m_driveType != RagdollRootControlBodySettings::DriveType::Spring );

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth( -1 );
            if ( ImGui::InputFloat( "##Stiffness", &cachedSettings.m_stiffness, 0.0f, 0.0f, "%.1f" ) )
            {
                cachedSettings.m_stiffness = Math::Clamp( cachedSettings.m_stiffness, 0.0f, 1000000.0f );
            }
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ScopedRagdollSettingsModification const sdm( this );
                realSettings.m_stiffness = cachedSettings.m_stiffness;
            }
            ImGuiX::ItemTooltip( "The stiffness of the spring used to correct the root body once the limit is exceeded" );

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth( -1 );
            if ( ImGui::InputFloat( "##Damping", &cachedSettings.m_damping, 0.0f, 0.0f, "%.1f" ) )
            {
                cachedSettings.m_damping = Math::Clamp( cachedSettings.m_damping, 0.0f, 1000000.0f );
            }
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ScopedRagdollSettingsModification const sdm( this );
                realSettings.m_damping = cachedSettings.m_damping;
            }
            ImGuiX::ItemTooltip( "The damping of the spring used to correct the root body once the limit is exceeded" );

            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::BeginDisabled( realSettings.m_driveType == RagdollRootControlBodySettings::DriveType::None );

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth( -1 );
            if ( ImGui::InputFloat( "##Max Distance", &cachedSettings.m_maxDistance, 0.0f, 0.0f, "%.3f" ) )
            {
                cachedSettings.m_maxDistance = Math::Clamp( cachedSettings.m_maxDistance, 0.0f, 5.0f );
            }
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ScopedRagdollSettingsModification const sdm( this );
                realSettings.m_maxDistance = cachedSettings.m_maxDistance;
            }
            ImGuiX::ItemTooltip( "The max distance the root body can drift from the control body" );

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth( -1 );
            if ( ImGui::InputFloat( "##Tolerance", &cachedSettings.m_tolerance, 0.0f, 0.0f, "%.3f" ) )
            {
                cachedSettings.m_tolerance = Math::Clamp( cachedSettings.m_tolerance, 0.0f, 1.0f );
            }
            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                ScopedRagdollSettingsModification const sdm( this );
                realSettings.m_tolerance = cachedSettings.m_tolerance;
            }
            ImGuiX::ItemTooltip( "The acceptance error tolerance before the position is corrected" );

            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::EndTable();
        }
    }

    void RagdollWorkspace::DrawJointSettingsTable( UpdateContext const& context, RagdollDefinition::Profile* pProfile )
    {
        static char const* const driveComboOptions[3] = { "Kinematic", "Spring", "Velocity" };

        EE_ASSERT( pProfile != nullptr );
        auto pRagdollDefinition = GetRagdollDefinition();

        ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
        if ( ImGui::BeginTable( "BodySettingsTable", 19, ImGuiTableFlags_BordersInnerV ) )
        {
            ImGui::TableSetupColumn( "Body", ImGuiTableColumnFlags_WidthFixed, 120 );
            ImGui::TableSetupColumn( "Mass", ImGuiTableColumnFlags_WidthFixed, 40 );
            ImGui::TableSetupColumn( "CCD", ImGuiTableColumnFlags_WidthFixed, 20 );
            ImGui::TableSetupColumn( "Self Collision", ImGuiTableColumnFlags_WidthFixed, 30 );
            ImGui::TableSetupColumn( "Int. Compliance", ImGuiTableColumnFlags_WidthFixed, 50 );
            ImGui::TableSetupColumn( "Ext. Compliance", ImGuiTableColumnFlags_WidthFixed, 50 );

            ImGui::TableSetupColumn( "Drive", ImGuiTableColumnFlags_WidthFixed, 75 );
            ImGui::TableSetupColumn( "Stiffness", ImGuiTableColumnFlags_WidthFixed, 50 );
            ImGui::TableSetupColumn( "Damping", ImGuiTableColumnFlags_WidthFixed, 50 );

            ImGui::TableSetupColumn( "Twist", ImGuiTableColumnFlags_WidthFixed, 24 );
            ImGui::TableSetupColumn( "Twist Lmt Min", ImGuiTableColumnFlags_WidthFixed, 60 );
            ImGui::TableSetupColumn( "Twist Lmt Max", ImGuiTableColumnFlags_WidthFixed, 60 );
            ImGui::TableSetupColumn( "Twist C-Dist", ImGuiTableColumnFlags_WidthFixed, 60 );

            ImGui::TableSetupColumn( "Swing", ImGuiTableColumnFlags_WidthFixed, 24 );
            ImGui::TableSetupColumn( "Swing Lmt Y", ImGuiTableColumnFlags_WidthFixed, 60 );
            ImGui::TableSetupColumn( "Swing Lmt Z", ImGuiTableColumnFlags_WidthFixed, 60 );
            ImGui::TableSetupColumn( "Swing C-Dist", ImGuiTableColumnFlags_WidthFixed, 60 );
            ImGui::TableSetupColumn( "Tang. Stiffness", ImGuiTableColumnFlags_WidthFixed, 50 );
            ImGui::TableSetupColumn( "Tang. Damping", ImGuiTableColumnFlags_WidthFixed, 50 );

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            int32_t const numBodies = pRagdollDefinition->GetNumBodies();
            int32_t const numJoints = numBodies - 1;
            for ( auto bodyIdx = 1; bodyIdx < numBodies; bodyIdx++ )
            {
                auto& cachedBodySettings = m_workingProfileCopy.m_bodySettings[bodyIdx];
                auto& realBodySettings = pProfile->m_bodySettings[bodyIdx];

                int32_t const jointIdx = bodyIdx - 1;
                auto& cachedJointSettings = m_workingProfileCopy.m_jointSettings[jointIdx];
                auto& realJointSettings = pProfile->m_jointSettings[jointIdx];

                //-------------------------------------------------------------------------
                // Body Settings
                //-------------------------------------------------------------------------

                ImGui::PushID( &cachedJointSettings );
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "%d. %s", bodyIdx, pRagdollDefinition->m_bodies[bodyIdx].m_boneID.c_str());

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputFloat( "##Mass", &cachedBodySettings.m_mass, 0.0f, 0.0f, "%.2f" ) )
                {
                    cachedBodySettings.m_mass = Math::Clamp( cachedBodySettings.m_mass, 0.01f, 1000000.0f );
                }
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realBodySettings.m_mass = cachedBodySettings.m_mass;
                }
                ImGuiX::ItemTooltip( "The mass of the body" );

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::Checkbox( "##CCD", &cachedBodySettings.m_enableCCD );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realBodySettings.m_enableCCD = cachedBodySettings.m_enableCCD;
                }
                ImGuiX::ItemTooltip( "Enable continuous collision detection for this body" );

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::Checkbox( "##SC", &cachedBodySettings.m_enableSelfCollision );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realBodySettings.m_enableSelfCollision = cachedBodySettings.m_enableSelfCollision;
                }
                ImGuiX::ItemTooltip( "Should this body collide with other bodies in the ragdoll" );

                //-------------------------------------------------------------------------
                // Joint Settings
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##IntCompliance", &cachedJointSettings.m_internalCompliance, Math::HugeEpsilon, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_internalCompliance = cachedJointSettings.m_internalCompliance;
                }
                ImGuiX::ItemTooltip( "Compliance determines the extent to which the joint resists acceleration caused by internal forces generated from other joints." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_internalCompliance;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_internalCompliance = value;
                            pProfile->m_jointSettings[i].m_internalCompliance = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##ExtCompliance", &cachedJointSettings.m_externalCompliance, Math::HugeEpsilon, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_externalCompliance = cachedJointSettings.m_externalCompliance;
                }
                ImGuiX::ItemTooltip( "Compliance determines the extent to which the joint resists acceleration caused by external forces such as gravity and contact forces" );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_externalCompliance;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_externalCompliance = value;
                            pProfile->m_jointSettings[i].m_externalCompliance = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );

                int32_t dt = (int32_t) cachedJointSettings.m_driveType;
                if( ImGui::Combo( "##DriveMode", &dt, driveComboOptions, 3 ) )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    cachedJointSettings.m_driveType = (RagdollJointSettings::DriveType) dt;
                    realJointSettings.m_driveType = cachedJointSettings.m_driveType;
                }

                ImGuiX::ItemTooltip( "The drive mode for this body's joint" );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        RagdollJointSettings::DriveType value = cachedJointSettings.m_driveType;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_driveType = value;
                            pProfile->m_jointSettings[i].m_driveType = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputFloat( "##Stiffness", &cachedJointSettings.m_stiffness, 0.0f, 0.0f, "%.1f" ) )
                {
                    cachedJointSettings.m_stiffness = Math::Clamp( cachedJointSettings.m_stiffness, 0.0f, 1000000.0f );
                }
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_stiffness = cachedJointSettings.m_stiffness;
                }
                ImGuiX::ItemTooltip( "The acceleration generated by the spring drive is proportional to this value and the angle between the drive target position and the current position" );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_stiffness;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_stiffness = value;
                            pProfile->m_jointSettings[i].m_stiffness = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputFloat( "##Damping", &cachedJointSettings.m_damping, 0.0f, 0.0f, "%.1f" ) )
                {
                    cachedJointSettings.m_damping = Math::Clamp( cachedJointSettings.m_damping, 0.0f, 1000000.0f );
                }
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_damping = cachedJointSettings.m_damping;
                }
                ImGuiX::ItemTooltip( "The acceleration generated by the spring drive is proportional to this value and the difference between the angular velocity of the joint and the target drive velocity." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_damping;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_damping = value;
                            pProfile->m_jointSettings[i].m_damping = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::Checkbox( "##Twist", &cachedJointSettings.m_twistLimitEnabled );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_twistLimitEnabled = cachedJointSettings.m_twistLimitEnabled;
                }
                ImGuiX::ItemTooltip( "Enable Twist Limits." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        bool const value = cachedJointSettings.m_twistLimitEnabled;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_twistLimitEnabled = value;
                            pProfile->m_jointSettings[i].m_twistLimitEnabled = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##TwistLimitMin", &cachedJointSettings.m_twistLimitMin, -179, cachedJointSettings.m_twistLimitMax - 1, "%.3f", ImGuiSliderFlags_AlwaysClamp );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    EE_ASSERT( cachedJointSettings.m_twistLimitMin < cachedJointSettings.m_twistLimitMax );
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_twistLimitMin = cachedJointSettings.m_twistLimitMin;
                }
                ImGuiX::ItemTooltip( "The lower limit value must be less than the upper limit if the limit is enabled." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_twistLimitMin;
                        float const maxTwistContactDistance = ( cachedJointSettings.m_twistLimitMax - value ) / 2;

                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            if ( value < m_workingProfileCopy.m_jointSettings[i].m_twistLimitMax )
                            {
                                m_workingProfileCopy.m_jointSettings[i].m_twistLimitMin = value;
                                if ( m_workingProfileCopy.m_jointSettings[i].m_twistLimitContactDistance >= maxTwistContactDistance )
                                {
                                    m_workingProfileCopy.m_jointSettings[i].m_twistLimitContactDistance = maxTwistContactDistance / 2;
                                }

                                pProfile->m_jointSettings[i].m_twistLimitMin = m_workingProfileCopy.m_jointSettings[i].m_twistLimitMin;
                                pProfile->m_jointSettings[i].m_twistLimitContactDistance = m_workingProfileCopy.m_jointSettings[i].m_twistLimitContactDistance;
                            }
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##TwistLimitMax", &cachedJointSettings.m_twistLimitMax, cachedJointSettings.m_twistLimitMin + 1, 179, "%.3f", ImGuiSliderFlags_AlwaysClamp );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    EE_ASSERT( cachedJointSettings.m_twistLimitMin < cachedJointSettings.m_twistLimitMax );
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_twistLimitMax = cachedJointSettings.m_twistLimitMax;
                }
                ImGuiX::ItemTooltip( "The lower limit value must be less than the upper limit if the limit is enabled." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_twistLimitMax;
                        float const maxTwistContactDistance = ( value - cachedJointSettings.m_twistLimitMin ) / 2;

                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            if ( value > m_workingProfileCopy.m_jointSettings[i].m_twistLimitMin )
                            {
                                m_workingProfileCopy.m_jointSettings[i].m_twistLimitMax = value;
                                if ( m_workingProfileCopy.m_jointSettings[i].m_twistLimitContactDistance >= maxTwistContactDistance )
                                {
                                    m_workingProfileCopy.m_jointSettings[i].m_twistLimitContactDistance = maxTwistContactDistance / 2;
                                }

                                pProfile->m_jointSettings[i].m_twistLimitMax = m_workingProfileCopy.m_jointSettings[i].m_twistLimitMax;
                                pProfile->m_jointSettings[i].m_twistLimitContactDistance = m_workingProfileCopy.m_jointSettings[i].m_twistLimitContactDistance;
                            }
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                // The contact distance should be less than half the distance between the upper and lower limits.
                float const maxTwistContactDistance = ( cachedJointSettings.m_twistLimitMax - cachedJointSettings.m_twistLimitMin ) / 2;
                if ( cachedJointSettings.m_twistLimitContactDistance >= maxTwistContactDistance )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    cachedJointSettings.m_twistLimitContactDistance = maxTwistContactDistance / 2;
                    realJointSettings.m_twistLimitContactDistance = cachedJointSettings.m_twistLimitContactDistance;
                }

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##TwistContactDistance", &cachedJointSettings.m_twistLimitContactDistance, 0, maxTwistContactDistance - Math::HugeEpsilon, "%.3f", ImGuiSliderFlags_AlwaysClamp );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_twistLimitContactDistance = cachedJointSettings.m_twistLimitContactDistance;
                }
                ImGuiX::ItemTooltip( "The contact distance should be less than half the distance between the upper and lower limits." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_twistLimitContactDistance;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_twistLimitContactDistance = value;
                            pProfile->m_jointSettings[i].m_twistLimitContactDistance = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::Checkbox( "##Swing", &cachedJointSettings.m_swingLimitEnabled );
                ImGuiX::ItemTooltip( "Enable Swing Limits." );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_swingLimitEnabled = cachedJointSettings.m_swingLimitEnabled;
                }

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );

                        bool const value = cachedJointSettings.m_swingLimitEnabled;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_swingLimitEnabled = value;
                            pProfile->m_jointSettings[i].m_swingLimitEnabled = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##SwingLimitY", &cachedJointSettings.m_swingLimitY, 1, 179, "%.1f", ImGuiSliderFlags_AlwaysClamp );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_swingLimitY = cachedJointSettings.m_swingLimitY;
                }
                ImGuiX::ItemTooltip( "Limit the allowed extent of rotation around the y-axis." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );

                        float const value = cachedJointSettings.m_swingLimitY;
                        float const maxSwingContactDistance = Math::Min( cachedJointSettings.m_swingLimitY, cachedJointSettings.m_swingLimitZ );

                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_swingLimitY = value;
                            if ( m_workingProfileCopy.m_jointSettings[i].m_swingLimitContactDistance >= maxSwingContactDistance )
                            {
                                m_workingProfileCopy.m_jointSettings[i].m_swingLimitContactDistance = maxSwingContactDistance / 2;
                            }

                            pProfile->m_jointSettings[i].m_swingLimitY = m_workingProfileCopy.m_jointSettings[i].m_swingLimitY;
                            pProfile->m_jointSettings[i].m_swingLimitContactDistance = m_workingProfileCopy.m_jointSettings[i].m_swingLimitContactDistance;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##SwingLimitZ", &cachedJointSettings.m_swingLimitZ, 1, 179, "%.1f", ImGuiSliderFlags_AlwaysClamp );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_swingLimitZ = cachedJointSettings.m_swingLimitZ;
                }
                ImGuiX::ItemTooltip( "Limit the allowed extent of rotation around the z-axis." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_swingLimitZ;
                        float const maxSwingContactDistance = Math::Min( cachedJointSettings.m_swingLimitY, cachedJointSettings.m_swingLimitZ );

                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_swingLimitZ = value;
                            if ( m_workingProfileCopy.m_jointSettings[i].m_swingLimitContactDistance >= maxSwingContactDistance )
                            {
                                m_workingProfileCopy.m_jointSettings[i].m_swingLimitContactDistance = maxSwingContactDistance / 2;
                            }

                            pProfile->m_jointSettings[i].m_swingLimitZ = m_workingProfileCopy.m_jointSettings[i].m_swingLimitZ;
                            pProfile->m_jointSettings[i].m_swingLimitContactDistance = m_workingProfileCopy.m_jointSettings[i].m_swingLimitContactDistance;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                // The contact distance should be less than either limit angle
                float const maxSwingContactDistance = Math::Min( cachedJointSettings.m_swingLimitY, cachedJointSettings.m_swingLimitZ );
                if ( cachedJointSettings.m_swingLimitContactDistance >= maxSwingContactDistance )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    cachedJointSettings.m_swingLimitContactDistance = maxSwingContactDistance - 1.0f;
                    realJointSettings.m_swingLimitContactDistance = cachedJointSettings.m_swingLimitContactDistance;
                }

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##SwingContactDistance", &cachedJointSettings.m_swingLimitContactDistance, 0, maxSwingContactDistance - Math::HugeEpsilon, "%.3f", ImGuiSliderFlags_AlwaysClamp );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_swingLimitContactDistance = cachedJointSettings.m_swingLimitContactDistance;
                }
                ImGuiX::ItemTooltip( "The contact distance should be less than either limit angle." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_swingLimitContactDistance;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_swingLimitContactDistance = value;
                            pProfile->m_jointSettings[i].m_swingLimitContactDistance = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputFloat( "##TangStiffness", &cachedJointSettings.m_tangentialStiffness, 0.0f, 0.0f, "%.1f" ) )
                {
                    cachedJointSettings.m_tangentialStiffness = Math::Clamp( cachedJointSettings.m_tangentialStiffness, 0.0f, 1000000.0f );
                }
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_tangentialStiffness = cachedJointSettings.m_tangentialStiffness;
                }
                ImGuiX::ItemTooltip( "The tangential stiffness for the swing limit cone." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_tangentialStiffness;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_tangentialStiffness = value;
                            pProfile->m_jointSettings[i].m_tangentialStiffness = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputFloat( "##TangDamping", &cachedJointSettings.m_tangentialDamping, 0.0f, 0.0f, "%.1f" ) )
                {
                    cachedJointSettings.m_tangentialDamping = Math::Clamp( cachedJointSettings.m_tangentialDamping, 0.0f, 1000000.0f );
                }
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realJointSettings.m_tangentialDamping = cachedJointSettings.m_tangentialDamping;
                }
                ImGuiX::ItemTooltip( "The tangential damping for the swing limit cone." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = cachedJointSettings.m_tangentialDamping;
                        for ( int32_t i = 0; i < numJoints; i++ )
                        {
                            m_workingProfileCopy.m_jointSettings[i].m_tangentialDamping = value;
                            pProfile->m_jointSettings[i].m_tangentialDamping = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            //-------------------------------------------------------------------------

            ImGui::EndTable();
        }
    }

    void RagdollWorkspace::DrawMaterialSettingsTable( UpdateContext const& context, RagdollDefinition::Profile* pProfile )
    {
        static char const* const comboOptions[4] = { "Average", "Min", "Multiply", "Max" };

        EE_ASSERT( pProfile != nullptr );
        auto pRagdollDefinition = GetRagdollDefinition();

        //-------------------------------------------------------------------------

        ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
        if ( ImGui::BeginTable( "MaterialSettingsTable", 6, ImGuiTableFlags_BordersInnerV ) )
        {
            ImGui::TableSetupColumn( "Body", ImGuiTableColumnFlags_WidthFixed, 120 );
            ImGui::TableSetupColumn( "Static Friction", ImGuiTableColumnFlags_WidthFixed, 100 );
            ImGui::TableSetupColumn( "Dynamic Friction", ImGuiTableColumnFlags_WidthFixed, 100 );
            ImGui::TableSetupColumn( "Restitution", ImGuiTableColumnFlags_WidthFixed, 100 );
            ImGui::TableSetupColumn( "Friction Combine", ImGuiTableColumnFlags_WidthFixed, 100 );
            ImGui::TableSetupColumn( "Restitution Combine", ImGuiTableColumnFlags_WidthFixed, 100 );

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            int32_t const numBodies = pRagdollDefinition->GetNumBodies();
            for ( auto bodyIdx = 0; bodyIdx < numBodies; bodyIdx++ )
            {
                auto& bodySettings = m_workingProfileCopy.m_materialSettings[bodyIdx];
                auto& realBodySettings = pProfile->m_materialSettings[bodyIdx];

                ImGui::PushID( &bodySettings );
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( pRagdollDefinition->m_bodies[bodyIdx].m_boneID.c_str() );

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::SliderFloat( "##StaticFriction", &bodySettings.m_staticFriction, 0, 10000, "%.3f", ImGuiSliderFlags_AlwaysClamp ) )
                {
                    bodySettings.m_staticFriction = Math::Clamp( bodySettings.m_staticFriction, 0.0f, 1000000.0f );
                }
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realBodySettings.m_staticFriction = bodySettings.m_staticFriction;
                }
                ImGuiX::ItemTooltip( "The coefficient of static friction." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = bodySettings.m_staticFriction;
                        for ( int32_t i = 0; i < numBodies; i++ )
                        {
                            m_workingProfileCopy.m_materialSettings[i].m_staticFriction = value;
                            pProfile->m_materialSettings[i].m_staticFriction = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                if( ImGui::SliderFloat( "##DynamicFriction", &bodySettings.m_dynamicFriction, 0, 10000, "%.3f", ImGuiSliderFlags_AlwaysClamp ) )
                {
                    bodySettings.m_dynamicFriction = Math::Clamp( bodySettings.m_dynamicFriction, 0.0f, 1000000.0f );
                }
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realBodySettings.m_dynamicFriction = bodySettings.m_dynamicFriction;
                }
                ImGuiX::ItemTooltip( "The coefficient of dynamic friction." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = bodySettings.m_dynamicFriction;
                        for ( int32_t i = 0; i < numBodies; i++ )
                        {
                            m_workingProfileCopy.m_materialSettings[i].m_dynamicFriction = value;
                            pProfile->m_materialSettings[i].m_dynamicFriction = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( "##Restitution", &bodySettings.m_restitution, 0, 1, "%.3f", ImGuiSliderFlags_AlwaysClamp );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realBodySettings.m_restitution = bodySettings.m_restitution;
                }
                ImGuiX::ItemTooltip( "A coefficient of 0 makes the object bounce as little as possible, higher values up to 1.0 result in more bounce." );

                if ( ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( "Set To All" ) )
                    {
                        ScopedRagdollSettingsModification const sdm( this );
                        float const value = bodySettings.m_restitution;
                        for ( int32_t i = 0; i < numBodies; i++ )
                        {
                            m_workingProfileCopy.m_materialSettings[i].m_restitution = value;
                            pProfile->m_materialSettings[i].m_restitution = value;
                        }
                    }
                    ImGui::EndPopup();
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                int32_t fc = (int32_t) bodySettings.m_frictionCombineMode;
                if( ImGui::Combo( "##FrictionCombine", &fc, comboOptions, 4 ) )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realBodySettings.m_frictionCombineMode = (PhysicsCombineMode) fc;
                    bodySettings.m_frictionCombineMode = (PhysicsCombineMode) fc;
                }
                ImGuiX::ItemTooltip( "Friction combine mode to set for this material" );

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth( -1 );
                int32_t rc = (int32_t) bodySettings.m_restitutionCombineMode;
                if( ImGui::Combo( "##RestitutionCombine", &rc, comboOptions, 4 ) )
                {
                    ScopedRagdollSettingsModification const sdm( this );
                    realBodySettings.m_restitutionCombineMode = (PhysicsCombineMode) rc;
                    bodySettings.m_restitutionCombineMode = (PhysicsCombineMode) rc;
                }
                ImGuiX::ItemTooltip( "Restitution combine mode for this material" );

                ImGui::PopID();
            }

            //-------------------------------------------------------------------------

            ImGui::EndTable();
        }
    }

    //-------------------------------------------------------------------------

    void RagdollWorkspace::DrawPreviewControlsWindow( UpdateContext const& context )
    {
        auto pRagdollDefinition = GetRagdollDefinition();

        if ( ImGui::Begin( m_previewControlsWindowName.c_str() ) )
        {
            if ( IsPreviewing() )
            {
                if ( ImGuiX::ColoredButton( Colors::MediumRed, Colors::White, EE_ICON_STOP" Stop Preview", ImVec2( -1, 0 ) ) )
                {
                    StopPreview();
                }
            }
            else
            {
                ImGui::BeginDisabled( !pRagdollDefinition->IsValid() && m_pMeshComponent != nullptr );
                if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_PLAY" Start Preview", ImVec2( -1, 0 ) ) )
                {
                    StartPreview( context );
                }
                ImGui::EndDisabled();
            }

            //-------------------------------------------------------------------------
            // General Settings
            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Start Transform" );
            ImGuiX::InputTransform( "StartTransform", m_previewStartTransform, -1.0f );

            if ( ImGui::Button( "Reset Start Transform", ImVec2( -1, 0 ) ) )
            {
                m_previewStartTransform = Transform::Identity;
            }

            //-------------------------------------------------------------------------
            // Ragdoll
            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Ragdoll" );

            if ( ImGui::Checkbox( "Enable Gravity", &m_enableGravity ) )
            {
                if ( IsPreviewing() )
                {
                    m_pRagdoll->SetGravityEnabled( m_enableGravity );
                }
            }

            if ( ImGui::Checkbox( "Enable Pose Following", &m_enablePoseFollowing  ) )
            {
                if ( IsPreviewing() )
                {
                    m_pRagdoll->SetPoseFollowingEnabled( m_enablePoseFollowing );
                }
            }

            ImGui::BeginDisabled( !IsPreviewing() );
            if ( ImGui::Button( "Reset Ragdoll State", ImVec2( -1, 0 ) ) )
            {
                m_pRagdoll->ResetState();
                m_pMeshComponent->SetWorldTransform( Transform::Identity );
                m_initializeRagdollPose = true;
            }
            ImGui::EndDisabled();

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Blend Weight:" );
            ImGui::SameLine();
            ImGui::SetNextItemWidth( -1 );
            ImGui::SliderFloat( "##PhysicsWeight", &m_physicsBlendWeight, 0, 1, "%.3f", ImGuiSliderFlags_NoInput );

            //-------------------------------------------------------------------------
            // PVD
            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "PhysX Visual Debugger" );

            ImGui::BeginDisabled( IsPreviewing() );
            ImGui::Checkbox( "Auto Connect to PVD", &m_autoConnectToPVD );
            ImGui::EndDisabled();

            ImGui::BeginDisabled( !IsPreviewing() );
            if ( m_pPhysicsSystem != nullptr && m_pPhysicsSystem->IsConnectedToPVD() )
            {
                if ( ImGui::Button( "Disconnect From PVD", ImVec2( -1, 0 ) ) )
                {
                    m_pPhysicsSystem->DisconnectFromPVD();
                }
            }
            else
            {
                if ( ImGui::Button( "Connect To PVD", ImVec2( -1, 0 ) ) )
                {
                    m_pPhysicsSystem->ConnectToPVD();
                }
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------
            // Animation
            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Animation" );

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Preview Anim:" );
            ImGui::SameLine();
            if ( m_resourceFilePicker.DrawResourcePicker( Animation::AnimationClip::GetStaticResourceTypeID(), &m_pPreviewAnimation.GetResourceID() ) )
            {
                // If we need to change resource ID, switch IDs
                ResourceID const selectedResourceID = m_resourceFilePicker.GetSelectedResourceID();
                if ( selectedResourceID != m_pPreviewAnimation.GetResourceID() )
                {
                    if ( m_pPreviewAnimation.IsValid() && !m_pPreviewAnimation.IsUnloaded() )
                    {
                        UnloadResource( &m_pPreviewAnimation );
                    }
                
                    m_pPreviewAnimation = selectedResourceID.IsValid() ? m_resourceFilePicker.GetSelectedResourceID() : nullptr;

                    if ( m_pPreviewAnimation.IsValid() )
                    {
                        LoadResource( &m_pPreviewAnimation );
                    }
                }
            }

            ImGui::Checkbox( "Enable Looping", &m_enableAnimationLooping );
            ImGui::Checkbox( "Apply Root Motion", &m_applyRootMotion );
            ImGui::Checkbox( "Auto Start", &m_autoPlayAnimation );

            //-------------------------------------------------------------------------

            bool const hasValidAndLoadedPreviewAnim = m_pPreviewAnimation.IsValid() && m_pPreviewAnimation.IsLoaded();
            ImGui::BeginDisabled( !IsPreviewing() || !hasValidAndLoadedPreviewAnim );

            ImVec2 const buttonSize( 20, 0 );
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

            ImGui::SameLine();
            ImGui::SetNextItemWidth( -1 );
            float currentAnimTime = m_animTime.ToFloat();
            if ( ImGui::SliderFloat( "##AnimTime", &currentAnimTime, 0, 1, "%.3f", ImGuiSliderFlags_NoInput ) )
            {
                m_isPlayingAnimation = false;
                m_animTime = currentAnimTime;
            }

            ImGui::EndDisabled();
        }
        ImGui::End();
    }

    void RagdollWorkspace::StartPreview( UpdateContext const& context )
    {
        auto pRagdollDefinition = GetRagdollDefinition();
        EE_ASSERT( m_pRagdoll == nullptr && pRagdollDefinition->IsValid() );
        EE_ASSERT( m_pSkeleton.IsLoaded() );

        // Skeleton is already loaded so the LoadResource will immediately set the loaded skeleton ptr
        LoadResource( &pRagdollDefinition->m_pSkeleton );
        pRagdollDefinition->CreateRuntimeData();

        //-------------------------------------------------------------------------

        m_pPhysicsSystem = context.GetSystem<PhysicsSystem>();
        if ( m_autoConnectToPVD )
        {
            m_pPhysicsSystem->ConnectToPVD();
        }

        auto pPhysicsWorldSystem = m_pWorld->GetWorldSystem<PhysicsWorldSystem>();
        m_pRagdoll = pPhysicsWorldSystem->GetScene()->CreateRagdoll( pRagdollDefinition, m_activeProfileID, 0 );
        m_pRagdoll->SetGravityEnabled( m_enableGravity );
        m_pRagdoll->SetPoseFollowingEnabled( m_enablePoseFollowing );

        m_initializeRagdollPose = true;

        //-------------------------------------------------------------------------

        if ( m_autoPlayAnimation )
        {
            m_animTime = 0.0f;
            m_isPlayingAnimation = true;
        }

        m_pPose = EE::New<Animation::Pose>( m_pSkeleton.GetPtr() );
        m_pFinalPose = EE::New<Animation::Pose>( m_pSkeleton.GetPtr() );

        m_pMeshComponent->SetWorldTransform( m_previewStartTransform );
    }

    void RagdollWorkspace::StopPreview()
    {
        EE_ASSERT( m_pRagdoll != nullptr );

        EE_ASSERT( m_pPhysicsSystem != nullptr );
        if ( m_pPhysicsSystem->IsConnectedToPVD() )
        {
            m_pPhysicsSystem->DisconnectFromPVD();
        }
        m_pPhysicsSystem = nullptr;

        //-------------------------------------------------------------------------

        m_pMeshComponent->ResetPose();
        m_pMeshComponent->FinalizePose();
        m_pMeshComponent->SetWorldTransform( Transform::Identity );

        auto pPhysicsWorldSystem = m_pWorld->GetWorldSystem<PhysicsWorldSystem>();
        m_pRagdoll->RemoveFromScene();
        EE::Delete( m_pRagdoll );

        auto pRagdollDefinition = GetRagdollDefinition();
        UnloadResource( &pRagdollDefinition->m_pSkeleton );

        //-------------------------------------------------------------------------

        EE::Delete( m_pPose );
        EE::Delete( m_pFinalPose );
    }

    void RagdollWorkspace::UpdateWorld( EntityWorldUpdateContext const& updateContext )
    {
        if ( !IsPreviewing() )
        {
            return;
        }

        Seconds const deltaTime = updateContext.GetDeltaTime();

        //-------------------------------------------------------------------------

        auto drawingCtx = GetDrawingContext();

        if ( updateContext.GetUpdateStage() == UpdateStage::PrePhysics )
        {
            // Update animation time and sample pose
            Percentage const previousAnimTime = m_animTime;
            bool const hasValidAndLoadedPreviewAnim = m_pPreviewAnimation.IsValid() && m_pPreviewAnimation.IsLoaded();
            if ( hasValidAndLoadedPreviewAnim )
            {
                if ( m_isPlayingAnimation )
                {
                    m_animTime = ( m_animTime + deltaTime / ( m_pPreviewAnimation->GetDuration() ) ).GetClamped( m_enableAnimationLooping );

                    // Allow reset
                    if ( !m_enableAnimationLooping && m_animTime == 1.0f )
                    {
                        m_isPlayingAnimation = false;
                    }

                    if ( !updateContext.IsWorldPaused() && m_applyRootMotion )
                    {
                        Transform const& WT = m_pMeshComponent->GetWorldTransform();
                        Transform const RMD = m_pPreviewAnimation->GetRootMotionDelta( previousAnimTime, m_animTime );
                        m_pMeshComponent->SetWorldTransform( RMD * WT );
                    }
                }

                m_pPreviewAnimation->GetPose( m_animTime, m_pPose );
            }
            else
            {
                m_pPose->Reset( Animation::Pose::Type::ReferencePose );
            }

            //-------------------------------------------------------------------------

            Transform const worldTransform = m_pMeshComponent->GetWorldTransform();

            // Draw the pose
            if ( m_drawAnimationPose )
            {
                drawingCtx.Draw( *m_pPose, worldTransform );
            }

            // Set ragdoll settings
            if ( m_refreshRagdollSettings )
            {
                m_pRagdoll->RefreshSettings();
                m_refreshRagdollSettings = false;
            }

            // Update ragdoll
            m_pPose->CalculateGlobalTransforms();
            if ( !updateContext.IsWorldPaused() )
            {
                m_pRagdoll->Update( updateContext.GetDeltaTime(), worldTransform, m_pPose, m_initializeRagdollPose );
                m_initializeRagdollPose = false;
            }
        }
        else if ( updateContext.GetUpdateStage() == UpdateStage::PostPhysics )
        {
            // Copy the anim pose
            m_pFinalPose->CopyFrom( m_pPose );

            // Retrieve the ragdoll pose
            Transform const worldTransform = m_pMeshComponent->GetWorldTransform();
            bool const killPreview = !m_pRagdoll->GetPose( worldTransform.GetInverse(), m_pPose );

            // Apply physics blend weight
            Animation::Blender::Blend( m_pFinalPose, m_pPose, m_physicsBlendWeight, TBitFlags<Animation::PoseBlendOptions>(), nullptr, m_pFinalPose);

            // Draw ragdoll pose
            if ( m_drawRagdoll )
            {
                drawingCtx.Draw( *m_pRagdoll );
            }

            // Update mesh pose
            m_pFinalPose->CalculateGlobalTransforms();
            m_pMeshComponent->SetPose( m_pFinalPose );
            m_pMeshComponent->FinalizePose();

            //-------------------------------------------------------------------------

            if ( killPreview )
            {
                EE_LOG_WARNING( "Physics", "Ragdoll", "Ragdoll destabilized" );
                StopPreview();
            }
        }
    }
}