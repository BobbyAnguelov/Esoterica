#include "ResourceEditor_Hitbox.h"
#include "EngineTools/PropertyGrid/PropertyGridTypeEditingRules.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Core/SystemDialogs.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "Engine/Hitbox/Hitbox_Instance.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE
{
    class HitboxShapeEditingRules : public PG::TTypeEditingRules<HitboxShape>
    {
        using PG::TTypeEditingRules<HitboxShape>::TTypeEditingRules;

        virtual HiddenState IsHidden( StringID const& propertyID ) override
        {
            if ( propertyID == StringID( "m_radius" ) )
            {
                if ( m_pTypeInstance->m_type != HitboxShape::Type::Capsule && m_pTypeInstance->m_type != HitboxShape::Type::Sphere )
                {
                    return HiddenState::Hidden;
                }
            }

            if ( propertyID == StringID( "m_halfHeight" ) )
            {
                if ( m_pTypeInstance->m_type != HitboxShape::Type::Capsule )
                {
                    return HiddenState::Hidden;
                }
            }

            if ( propertyID == StringID( "m_boxExtents" ) )
            {
                if ( m_pTypeInstance->m_type != HitboxShape::Type::Box )
                {
                    return HiddenState::Hidden;
                }
            }

            return HiddenState::Unhandled;
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( HitboxShape, HitboxShapeEditingRules );

    //-------------------------------------------------------------------------

    EE_RESOURCE_EDITOR_FACTORY( HitboxEditorFactory, HitboxDefinition, HitboxEditor );

    //-------------------------------------------------------------------------

    HitboxEditor::HitboxEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld )
        : TResourceEditor<HitboxDefinition>( pToolsContext, resourceID, pWorld )
        , m_propertyGrid( pToolsContext )
        , m_setupResourcePicker( *pToolsContext )
        , m_animPicker( *pToolsContext )
    {
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowNonUniformScale, true );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowCoordinateSpaceSwitching, true );
        m_gizmo.SetMode( ImGuiX::Gizmo::Mode::Translation );

        auto OnPreEdit = [this] ( PropertyEditInfo const& info ) { BeginDataFileModification(); };
        auto OnPostEdit = [this] ( PropertyEditInfo const& info ) { EE_ASSERT( m_pActiveUndoableAction != nullptr ); EndDataFileModification(); };

        m_preEditEventBindingID = m_propertyGrid.OnPreEdit().Bind( OnPreEdit );
        m_postEditEventBindingID = m_propertyGrid.OnPostEdit().Bind( OnPostEdit );

        // Setup Resource
        //-------------------------------------------------------------------------

        // Set a custom filter for the generated options
        auto CustomResourceFilter = []( Resource::ResourceDescriptor const* pDesc )
        {
            if ( pDesc->GetCompiledResourceTypeID() == Render::SkeletalMesh::GetStaticResourceTypeID() )
            {
                return true;
            }

            if ( pDesc->GetCompiledResourceTypeID() == Animation::Skeleton::GetStaticResourceTypeID() )
            {
                return true;
            }

            return false;
        };

        m_setupResourcePicker.SetCustomResourceFilter( CustomResourceFilter );

        // Preview Anim
        //-------------------------------------------------------------------------

        auto CustomClipResourceFilter = [this] ( Resource::ResourceDescriptor const* pDesc )
        {
            if ( IsUsingMeshAsSetupResource() )
            {
                return true;
            }

            auto pClipDesc = Cast<Animation::AnimationClipResourceDescriptor>( pDesc );
            return pClipDesc->m_skeleton.GetResourceID() == m_setupResource.GetResourceID();
        };

        m_animPicker.SetRequiredResourceType( Animation::AnimationClip::GetStaticResourceTypeID() );
        m_animPicker.SetCustomResourceFilter( CustomClipResourceFilter );
    }

    HitboxEditor::~HitboxEditor()
    {
        EE_ASSERT( !m_setupResource.IsSet() || m_setupResource.IsUnloaded() );
        EE_ASSERT( m_pSocketTreeRoot == nullptr );

        m_propertyGrid.OnPreEdit().Unbind( m_preEditEventBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_postEditEventBindingID );
    }

    //-------------------------------------------------------------------------

    void HitboxEditor::Initialize( UpdateContext const& context )
    {
        TResourceEditor<HitboxDefinition>::Initialize( context );

        CreateToolWindow( "Shapes", [this] ( UpdateContext const& context, bool isFocused ) { DrawShapeEditorWindow( context, isFocused ); } );
        CreateToolWindow( "Details", [this] ( UpdateContext const& context, bool isFocused ) { DrawDetailsWindow( context, isFocused ); } );
        CreateToolWindow( "Preview Controls", [this] ( UpdateContext const& context, bool isFocused ) { DrawPreviewControlsWindow( context, isFocused ); } );

        static Transform const cameraDefaultTransform = Transform( Quaternion( EulerAngles( 10.0f, 0.0f, -180.0f ) ), Vector( 0, -2, 1.5f, 1 ) );
        SetCameraTransform( cameraDefaultTransform );
    }

    void HitboxEditor::Shutdown( UpdateContext const& context )
    {
        // Unload preview animation
        if ( m_previewAnimation.IsSet() && !m_previewAnimation.IsUnloaded() )
        {
            UnloadResource( &m_previewAnimation );
        }

        TResourceEditor<HitboxDefinition>::Shutdown( context );
    }

    void HitboxEditor::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID = 0, bottomLeftDockID = 0, rightDockID = 0, viewportDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.25f, &leftDockID, &viewportDockID );
        ImGui::DockBuilderSplitNode( leftDockID, ImGuiDir_Up, 0.67f, &leftDockID, &bottomLeftDockID );
        ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Right, 0.33f, &rightDockID, &viewportDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Shapes" ).c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Details" ).c_str(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Preview Controls" ).c_str(), rightDockID );
    }

    //-------------------------------------------------------------------------

    void HitboxEditor::CreatePreviewEntity( bool setSkeleton )
    {
        EE_ASSERT( IsDataFileLoaded() );
        EE_ASSERT( m_setupResource.IsLoaded() );

        ResourceID meshResourceID;

        if( IsUsingSkeletonAsSetupResource() )
        {
            // Load resource descriptor for skeleton to get the preview mesh
            auto resourceDescPath = GetFileSystemPath( m_setupResource.GetDataPath() );
            Animation::SkeletonResourceDescriptor skeletonResourceDesc;
            Log log;
            bool const result = Resource::ResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, log, resourceDescPath, skeletonResourceDesc );
            EE_ASSERT( result );

            meshResourceID = skeletonResourceDesc.m_previewMesh.GetResourceID();
        }
        else if( IsUsingMeshAsSetupResource() )
        {
            meshResourceID = m_setupResource.GetResourceID();
        }

        // Create preview entity
        if ( meshResourceID.IsValid() )
        {
            m_pMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
            m_pMeshComponent->SetMesh( meshResourceID );
            m_pMeshComponent->SetVisible( true );

            if ( setSkeleton && m_previewAnimation.IsLoaded() )
            {
                m_pMeshComponent->SetSkeleton( m_previewAnimation->GetSkeleton()->GetResourceID() );
            }

            m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
            m_pPreviewEntity->AddComponent( m_pMeshComponent );
            AddEntityToWorld( m_pPreviewEntity );
        }
    }

    void HitboxEditor::DestroyPreviewEntity()
    {
        if ( m_pPreviewEntity != nullptr )
        {
            DestroyEntityInWorld( m_pPreviewEntity );
            m_pMeshComponent = nullptr;
        }
    }

    void HitboxEditor::OnPreviewStarted()
    {
        EE_ASSERT( m_pHitbox == nullptr && IsResourceLoaded() );

        if ( m_previewAnimation.IsLoaded() )
        {
            if ( m_autoPlayAnimation )
            {
                m_animTime = 0.0f;
                m_isPlayingAnimation = true;
            }

            DestroyPreviewEntity();
            CreatePreviewEntity( true );

            if ( m_pMeshComponent->HasMeshResourceSet() && m_pMeshComponent->HasSkeletonResourceSet() )
            {
                m_pPose = EE::New<Animation::Pose>( m_previewAnimation->GetSkeleton() );
                CalculatePreviewPose( 0.0f );
            }
        }

        HitboxDefinition const* pHitboxDefinition = GetPreviewResource<HitboxDefinition>();
        m_pHitbox = EE::New<Hitbox>( pHitboxDefinition );
    }

    void HitboxEditor::OnPreviewStopped()
    {
        EE_ASSERT( m_pHitbox != nullptr );

        if ( m_pMeshComponent->HasMeshResourceSet() )
        {
            m_pMeshComponent->ResetPose();
            m_pMeshComponent->FinalizePose();
        }

        EE::Delete( m_pPose );
        EE::Delete( m_pHitbox );

        //-------------------------------------------------------------------------

        DestroyPreviewEntity();
        CreatePreviewEntity();
    }

    void HitboxEditor::SetAndLoadSetupResource()
    {
        EE_ASSERT( !m_setupResource.IsSet() );

        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        if ( pHitboxDescriptor->m_mesh.IsSet() )
        {
            m_setupResource = pHitboxDescriptor->m_mesh.GetResourceID();
        }
        else if ( pHitboxDescriptor->m_skeleton.IsSet() )
        {
            m_setupResource = pHitboxDescriptor->m_skeleton.GetResourceID();
        }

        m_setupResourcePicker.SetResourceID( m_setupResource.GetResourceID() );

        //-------------------------------------------------------------------------

        if ( m_setupResource.IsSet() )
        {
            LoadResource( &m_setupResource );
        }
        else
        {
            MessageDialog::Error( "Invalid Descriptor", "Descriptor doesnt have a setup resource set!" );
        }
    }

    void HitboxEditor::UnloadAndClearSetupResource()
    {
        DestroyPreviewEntity();

        if ( m_setupResource.WasRequested() )
        {
            UnloadResource( &m_setupResource );
        }
        m_setupResource.Clear();
    }

    void HitboxEditor::OnDataFileLoadCompleted()
    {
        if ( IsDataFileLoaded() )
        {
            SetAndLoadSetupResource();
        }
    }

    void HitboxEditor::OnDataFileUnload()
    {
        DestroyPreviewEntity();
        UnloadAndClearSetupResource();
        m_selectedItemID.Clear();
    }

    void HitboxEditor::OnResourceUnload( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_setupResource )
        {
            DestroySocketTree();
            DestroyPreviewEntity();
        }
    }

    void HitboxEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_setupResource && m_setupResource.IsLoaded() )
        {
            CreateSocketTree();
            CreatePreviewEntity();
        }
    }

    void HitboxEditor::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        UnloadAndClearSetupResource();
        m_commandStack.PushCommand( [this] () { SetAndLoadSetupResource(); } );
    }

    //-------------------------------------------------------------------------

    void HitboxEditor::CalculatePreviewPose( Seconds deltaTime )
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
            }

            m_previewAnimation->GetPose( m_animTime, m_pPose );
        }
        else
        {
            m_pPose->Reset( Animation::Pose::Init::ReferencePose );
        }

        m_pPose->CalculateModelSpaceTransforms();
    }

    void HitboxEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        // Execute any schedule commands
        //-------------------------------------------------------------------------

        if ( m_commandStack.HasCommands() )
        {
            m_commandStack.ExecuteCommands();
            m_propertyGrid.SetTypeToEdit( nullptr );
        }

        // Validate Selection
        //-------------------------------------------------------------------------

        if ( !IsDataFileLoaded() && m_selectedItemID.IsValid() )
        {
            if ( GetSelectedShape() == nullptr )
            {
                m_selectedItemID.Clear();
            }
        }

        // Preview
        //-------------------------------------------------------------------------

        if ( IsPreviewing() )
        {
            Seconds const scaledDeltaTime = context.GetDeltaTime() * m_pWorld->GetTimeScale();

            if ( m_previewAnimation.IsLoaded() && m_pPose != nullptr )
            {
                // Calculate the desired input pose
                Seconds const animDeltaTime = m_pWorld->IsPaused() ? Seconds( 0.0f ) : scaledDeltaTime;
                CalculatePreviewPose( animDeltaTime );

                // Update instance
                if ( m_pMeshComponent->IsInitialized() )
                {
                    m_pMeshComponent->SetPose( m_pPose );
                    m_pMeshComponent->FinalizePose();
                }

                m_pHitbox->Update( scaledDeltaTime, Transform::Identity, m_pPose );
            }
            else
            {
                m_pHitbox->Update( scaledDeltaTime, m_pMeshComponent );
            }
        }
    }

    //-------------------------------------------------------------------------

    bool HitboxEditor::IsUsingSkeletonAsSetupResource() const
    {
        if ( m_setupResource.IsSet() )
        {
            return m_setupResource.GetResourceTypeID() == Animation::Skeleton::GetStaticResourceTypeID();
        }

        return false;
    }

    bool HitboxEditor::IsUsingMeshAsSetupResource() const
    {
        if ( m_setupResource.IsSet() )
        {
            return m_setupResource.GetResourceTypeID() == Render::SkeletalMesh::GetStaticResourceTypeID();
        }

        return false;
    }

    int32_t HitboxEditor::GetSocketIndex( StringID socketID ) const
    {
        if ( m_setupResource.IsLoaded() )
        {
            if ( IsUsingMeshAsSetupResource() )
            {
                auto pMesh = m_setupResource.GetPtr<Render::SkeletalMesh>();
                return pMesh->GetBoneIndex( socketID );
            }
            else
            {
                auto pSkeleton = m_setupResource.GetPtr<Animation::Skeleton>();
                return pSkeleton->GetBoneIndex( socketID );
            }
        }

        return InvalidIndex;
    }

    void HitboxEditor::CreateSocketTree()
    {
        TVector<SocketInfo*> socketInfos;

        if ( IsUsingMeshAsSetupResource() )
        {
            auto pMesh = m_setupResource.GetPtr<Render::SkeletalMesh>();

            // Create all infos
            int32_t const numBones = pMesh->GetNumBones();
            for ( auto i = 0; i < numBones; i++ )
            {
                auto& pSocketInfo = socketInfos.emplace_back( EE::New<SocketInfo>() );
                pSocketInfo->m_boneID = pMesh->GetBoneID( i );
                pSocketInfo->m_boneIdx = i;
            }

            // Create hierarchy
            for ( auto i = 1; i < numBones; i++ )
            {
                int32_t const parentBoneIdx = pMesh->GetParentBoneIndex( i );
                EE_ASSERT( parentBoneIdx != InvalidIndex );
                socketInfos[parentBoneIdx]->m_children.emplace_back( socketInfos[i] );
            }

            // Set root
            m_pSocketTreeRoot = socketInfos[0];
        }
        else
        {
            auto pSkeleton = m_setupResource.GetPtr<Animation::Skeleton>();

            // Create all infos
            int32_t const numBones = pSkeleton->GetNumBones();
            for ( auto i = 0; i < numBones; i++ )
            {
                auto& pSocketInfo = socketInfos.emplace_back( EE::New<SocketInfo>() );
                pSocketInfo->m_boneID = pSkeleton->GetBoneID( i );
                pSocketInfo->m_boneIdx = i;
            }

            // Create hierarchy
            for ( auto i = 1; i < numBones; i++ )
            {
                int32_t const parentBoneIdx = pSkeleton->GetParentBoneIndex( i );
                EE_ASSERT( parentBoneIdx != InvalidIndex );
                socketInfos[parentBoneIdx]->m_children.emplace_back( socketInfos[i] );
            }

            // Set root
            m_pSocketTreeRoot = socketInfos[0];
        }
    }

    void HitboxEditor::DestroySocketTree()
    {
        if ( m_pSocketTreeRoot != nullptr )
        {
            m_pSocketTreeRoot->DestroyChildren();
            EE::Delete( m_pSocketTreeRoot );
        }
    }

    bool HitboxEditor::IsShapeSelected() const
    {
        if ( !m_selectedItemID.IsValid() )
        {
            return false;
        }

        auto const pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        for ( int32_t i = 0; i < pHitboxDescriptor->m_shapes.size(); i++ )
        {
            if ( pHitboxDescriptor->m_shapes[i].m_ID == m_selectedItemID )
            {
                return true;
            }
        }

        return false;
    }

    HitboxShape* HitboxEditor::GetShape( UUID const& ID )
    {
        EE_ASSERT( IsDataFileLoaded() );

        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        for ( auto& shape : pHitboxDescriptor->m_shapes )
        {
            if ( shape.m_ID == ID )
            {
                return &shape;
            }
        }

        return nullptr;
    }

    Transform HitboxEditor::GetSelectedElementTransform() const
    {
        if ( IsShapeSelected() )
        {
            return GetSelectedShapeTransform();
        }
        else
        {
            return Transform::Identity;
        }
    }

    Transform HitboxEditor::GetSocketTransform( StringID socketID ) const
    {
        Transform socketTransform = Transform::Identity;

        if ( IsUsingMeshAsSetupResource() )
        {
            auto pMesh = m_setupResource.GetPtr<Render::SkeletalMesh>();
            int32_t const boneIdx = pMesh->GetBoneIndex( socketID );
            if ( boneIdx != InvalidIndex )
            {
                socketTransform = pMesh->GetBindPose()[boneIdx];
            }
        }
        else if ( IsUsingSkeletonAsSetupResource() )
        {
            auto pSkeleton = m_setupResource.GetPtr<Animation::Skeleton>();
            int32_t const boneIdx = pSkeleton->GetBoneIndex( socketID );
            if ( boneIdx != InvalidIndex )
            {
                socketTransform = pSkeleton->GetBoneModelSpaceTransform( boneIdx );
            }
        }

        return socketTransform;
    }

    Transform HitboxEditor::GetShapeTransform( UUID const& ID ) const
    {
        Transform shapeTransform = Transform::Identity;

        auto const pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        for ( auto const& shape : pHitboxDescriptor->m_shapes )
        {
            if ( shape.m_ID == ID )
            {
                Transform socketTransform = GetSocketTransform( shape.m_socketID );
                shapeTransform = shape.m_offset * socketTransform;
                break;
            }
        }

        return shapeTransform;
    }

    void HitboxEditor::SetShapeTransform( UUID const& ID, Transform const& transform )
    {
        auto const pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        for ( auto& shape : pHitboxDescriptor->m_shapes )
        {
            if ( shape.m_ID == ID )
            {
                Transform socketTransform = GetSocketTransform( shape.m_socketID );

                ScopedDataFileModification sm( this );
                shape.m_offset = transform * socketTransform.GetInverse();
                break;
            }
        }
    }

    void HitboxEditor::ScaleShape( UUID const& ID, Vector const& scaleDelta )
    {
        auto pShape = GetShape( ID );
        EE_ASSERT( pShape != nullptr );

        if ( pShape->m_type == HitboxShape::Type::Box )
        {
            Vector extents = pShape->m_boxExtents + scaleDelta;
            pShape->m_boxExtents = Vector::Max( Vector( HitboxShape::s_minimumWidthOrRadius ), extents );
        }
        else if ( pShape->m_type == HitboxShape::Type::Sphere )
        {
            pShape->m_radius = Math::Max( pShape->m_radius + scaleDelta.GetX(), HitboxShape::s_minimumWidthOrRadius );
        }
        else if ( pShape->m_type == HitboxShape::Type::Capsule )
        {
            float const radiusScale = ( scaleDelta.GetX() ) ? scaleDelta.GetX() : scaleDelta.GetY();

            pShape->m_radius = Math::Max( pShape->m_radius + radiusScale, HitboxShape::s_minimumWidthOrRadius );
            pShape->m_halfHeight = Math::Max( pShape->m_halfHeight + scaleDelta.GetZ(), HitboxShape::s_minimumWidthOrRadius );
        }
    }

    bool HitboxEditor::GetShapeCreationTransforms( StringID socketID, Transform& socketTransform, Transform& socketChildTransform )
    {
        bool hasChildTransform = false;

        if ( IsUsingMeshAsSetupResource() )
        {
            auto pMesh = m_setupResource.GetPtr<Render::SkeletalMesh>();
            int32_t const boneIdx = pMesh->GetBoneIndex( socketID );
            if ( boneIdx != InvalidIndex )
            {
                socketTransform = pMesh->GetBindPoseTransform( boneIdx );
            }

            // Get longest child transform
            float maxLength = 0;
            for ( int32_t childBoneIdx = boneIdx; childBoneIdx < pMesh->GetNumBones(); childBoneIdx++ )
            {
                if ( pMesh->IsDirectChildBoneOf( boneIdx, childBoneIdx ) )
                {
                    Transform childTransform = pMesh->GetBindPoseTransform( childBoneIdx );
                    float const boneLength = childTransform.GetTranslation().GetDistance3( socketTransform.GetTranslation() );
                    if ( boneLength > maxLength )
                    {
                        maxLength = boneLength;
                        socketChildTransform = childTransform;
                        hasChildTransform = true;
                    }
                }
            }
        }
        else if ( IsUsingSkeletonAsSetupResource() )
        {
            auto pSkeleton = m_setupResource.GetPtr<Animation::Skeleton>();
            int32_t const boneIdx = pSkeleton->GetBoneIndex( socketID );
            if ( boneIdx != InvalidIndex )
            {
                socketTransform = pSkeleton->GetBoneModelSpaceTransform( boneIdx );
            }

            // Get longest child transform
            float maxLength = 0;
            for ( int32_t childBoneIdx = boneIdx; childBoneIdx < pSkeleton->GetNumBones(); childBoneIdx++ )
            {
                if ( pSkeleton->IsDirectChildBoneOf( boneIdx, childBoneIdx ) )
                {
                    Transform childTransform = pSkeleton->GetBoneModelSpaceTransform( childBoneIdx );
                    float const boneLength = childTransform.GetTranslation().GetDistance3( socketTransform.GetTranslation() );
                    if ( boneLength > maxLength )
                    {
                        maxLength = boneLength;
                        socketChildTransform = childTransform;
                        hasChildTransform = true;
                    }
                }
            }
        }

        return hasChildTransform;
    }

    void HitboxEditor::CreateBoxShape( StringID socketID )
    {
        ScopedDataFileModification sm( this );
        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        auto& shape = pHitboxDescriptor->m_shapes.emplace_back();
        shape.m_type = HitboxShape::Type::Box;
        shape.m_socketID = socketID;

        // Initial Offset
        //-------------------------------------------------------------------------

        Transform socketTransform;
        Transform childTransform;
        bool hasChildTransform = GetShapeCreationTransforms( socketID, socketTransform, childTransform );
        if ( hasChildTransform )
        {
            Vector const boneStart = socketTransform.GetTranslation();
            Vector const boneEnd = childTransform.GetTranslation();
            Vector boneDir; float boneLength = 0;
            ( boneEnd - boneStart ).ToDirectionAndLength3( boneDir, boneLength );

            if ( !Math::IsNearZero( boneLength ) )
            {
                float const width = Math::Max( boneLength * 0.05f, 0.05f );
                float const boneHalfLength = boneLength / 2.0f;
                shape.m_boxExtents = Vector( width, width, boneHalfLength );

                Quaternion const orientation = Quaternion::FromRotationBetweenUnitVectors( boneDir, Vector::WorldUp );
                Transform const boxTransform( orientation, boneStart + ( boneDir * boneHalfLength ) );
                shape.m_offset = boxTransform * socketTransform.GetInverse();
            }
        }
    }

    void HitboxEditor::CreateSphereShape( StringID socketID )
    {
        ScopedDataFileModification sm( this );
        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        auto& shape = pHitboxDescriptor->m_shapes.emplace_back();
        shape.m_type = HitboxShape::Type::Sphere;
        shape.m_socketID = socketID;

        // Initial Offset
        //-------------------------------------------------------------------------

        Transform socketTransform;
        Transform childTransform;
        bool hasChildTransform = GetShapeCreationTransforms( socketID, socketTransform, childTransform );

        if ( hasChildTransform )
        {
            Vector const boneStart = socketTransform.GetTranslation();
            Vector const boneEnd = childTransform.GetTranslation();
            float boneLength = ( boneEnd - boneStart ).GetLength3();
            shape.m_radius = Math::Max( boneLength * 0.05f, 0.05f );
        }
    }

    void HitboxEditor::CreateCapsuleShape( StringID socketID )
    {
        ScopedDataFileModification sm( this );
        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        auto& shape = pHitboxDescriptor->m_shapes.emplace_back();
        shape.m_type = HitboxShape::Type::Capsule;
        shape.m_socketID = socketID;

        // Initial Offset
        //-------------------------------------------------------------------------

        Transform socketTransform;
        Transform childTransform;
        bool hasChildTransform = GetShapeCreationTransforms( socketID, socketTransform, childTransform );
        if ( hasChildTransform )
        {
            Vector const boneStart = socketTransform.GetTranslation();
            Vector const boneEnd = childTransform.GetTranslation();
            Vector boneDir; float boneLength = 0;
            ( boneEnd - boneStart ).ToDirectionAndLength3( boneDir, boneLength );

            if ( !Math::IsNearZero( boneLength ) )
            {
                shape.m_radius = Math::Max( boneLength * 0.05f, 0.05f );
                shape.m_halfHeight = Math::Max( ( boneLength / 2.0f ) - shape.m_radius, 0.01f );

                Quaternion const orientation = Quaternion::FromRotationBetweenUnitVectors( boneDir, Vector::WorldUp );
                Transform const boxTransform( orientation, boneStart + ( boneDir * shape.m_halfHeight ) );
                shape.m_offset = boxTransform * socketTransform.GetInverse();
            }
        }
    }

    //-------------------------------------------------------------------------

    void HitboxEditor::DrawToolbar( UpdateContext const& context )
    {
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float const liveDebugButtonWidth = 150;
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
            ImGui::BeginDisabled( !IsResourceLoaded() || !m_previewAnimation.IsLoaded() || IsPreviewingOrWaitingForPreview() );
            if ( ImGuiX::FlatIconButton( EE_ICON_PLAY, "Preview", Colors::Lime, ImVec2( liveDebugButtonWidth, 0 ), true ) )
            {
                RequestPreview();
            }
            ImGui::EndDisabled();
        }
    }

    void HitboxEditor::DrawShapeEditorWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsDataFileLoaded() )
        {
            return;
        }

        // Setup Resource Picker
        //-------------------------------------------------------------------------

        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();

        if ( m_setupResourcePicker.UpdateAndDraw() )
        {
            UnloadAndClearSetupResource();

            ScopedDataFileModification sm( this );

            pHitboxDescriptor->m_mesh.Clear();
            pHitboxDescriptor->m_skeleton.Clear();

            ResourceID const selectedResourceID = m_setupResourcePicker.GetResourceID();
            if ( selectedResourceID.GetResourceTypeID() == Render::SkeletalMesh::GetStaticResourceTypeID() )
            {
                pHitboxDescriptor->m_mesh = selectedResourceID;
            }
            else if ( selectedResourceID.GetResourceTypeID() == Animation::Skeleton::GetStaticResourceTypeID() )
            {
                pHitboxDescriptor->m_skeleton = selectedResourceID;
            }

            m_commandStack.PushCommand( [this] () { SetAndLoadSetupResource(); } );
        }

        // Shape List
        //-------------------------------------------------------------------------

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );
        if ( ImGui::BeginTable( "Shapes", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders, ImGui::GetContentRegionAvail() ) )
        {
            DrawShapesForSocket( InvalidIndex );

            if ( m_pSocketTreeRoot != nullptr )
            {
                DrawSocketTreeItem( m_pSocketTreeRoot );
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        // Camera
        //-------------------------------------------------------------------------

        if ( isFocused && m_selectedItemID.IsValid() && ImGui::IsKeyPressed( ImGuiKey_F ) )
        {
            Transform const selectedElementTransform = GetSelectedElementTransform();
            FocusCameraView( selectedElementTransform.GetTranslation(), 2.0f );
        }
    }

    ImRect HitboxEditor::DrawSocketTreeItem( SocketInfo* pSocket )
    {
        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        EE_ASSERT( pHitboxDescriptor != nullptr && pHitboxDescriptor->IsValid() );
        EE_ASSERT( pSocket != nullptr );

        //-------------------------------------------------------------------------

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // Socket Label
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( pSocket->m_isExpanded );
        int32_t treeNodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_LabelSpanAllColumns;

        if ( pSocket->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        bool const isSelected = pSocket->m_ID == m_selectedItemID;
        if ( isSelected )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        pSocket->m_isExpanded = ImGui::TreeNodeEx( pSocket->m_boneID.c_str(), treeNodeFlags );

        // Context Menu
        //-------------------------------------------------------------------------

        if ( ImGui::BeginPopupContextItem() )
        {
            if ( ImGui::MenuItem( EE_ICON_CUBE_OUTLINE" Box" ) )
            {
                CreateBoxShape( pSocket->m_boneID );
            }

            if ( ImGui::MenuItem( EE_ICON_SPHERE" Sphere" ) )
            {
                CreateSphereShape( pSocket->m_boneID );
            }

            if ( ImGui::MenuItem( EE_ICON_PILL" Capsule" ) )
            {
                CreateCapsuleShape( pSocket->m_boneID );
            }

            ImGui::EndPopup();
        }

        // Selection
        //-------------------------------------------------------------------------

        if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenBlockedByPopup ) )
        {
            if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
            {
                m_selectedItemID = pSocket->m_ID;
            }
        }

        // Draw Shapes
        //-------------------------------------------------------------------------

        DrawShapesForSocket( pSocket->m_boneIdx );

        // Draw Child Sockets
        //-------------------------------------------------------------------------

        ImRect const nodeRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

        if ( pSocket->m_isExpanded )
        {
            ImU32 const TreeLineColor = ImGui::GetColorU32( ImGuiCol_TextDisabled );
            float const SmallOffsetX = 2;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
            verticalLineStart.x += SmallOffsetX;
            ImVec2 verticalLineEnd = verticalLineStart;

            for ( SocketInfo* pChild : pSocket->m_children )
            {
                const float HorizontalTreeLineSize = 4.0f;
                const ImRect childRect = DrawSocketTreeItem( pChild );
                const float midpoint = ( childRect.Min.y + childRect.Max.y ) / 2.0f;
                drawList->AddLine( ImVec2( verticalLineStart.x, midpoint ), ImVec2( verticalLineStart.x + HorizontalTreeLineSize, midpoint ), TreeLineColor );
                verticalLineEnd.y = midpoint;
            }

            drawList->AddLine( verticalLineStart, verticalLineEnd, TreeLineColor );
            ImGui::TreePop();
        }

        return nodeRect;
    }

    void HitboxEditor::DrawShapesForSocket( int32_t socketIndex )
    {
        constexpr float const horizontalTreeLineSize = 4.0f;

        InlineString itemLabel;

        auto const pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        for ( int32_t i = 0; i < pHitboxDescriptor->m_shapes.size(); i++ )
        {
            HitboxShape const& shape = pHitboxDescriptor->m_shapes[i];
            int32_t const shapeSocketID = GetSocketIndex( shape.m_socketID );
            if ( shapeSocketID != socketIndex )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImU32 const treeLineColor = ImGui::GetColorU32( ImGuiCol_TextDisabled );
            float const smallOffsetX = 2;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
            verticalLineStart.x += smallOffsetX;
            ImVec2 verticalLineEnd = verticalLineStart;

            int32_t treeNodeFlags = ImGuiTreeNodeFlags_LabelSpanAllColumns;
            bool const isSelected = m_selectedItemID == shape.m_ID;
            if ( isSelected )
            {
                treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
            }

            // Label
            //-------------------------------------------------------------------------

            {
                switch ( shape.m_type )
                {
                    case HitboxShape::Type::Capsule:
                    {
                        itemLabel.sprintf( EE_ICON_PILL" Capsule##%d", i );
                    }
                    break;

                    case HitboxShape::Type::Box:
                    {
                        itemLabel.sprintf( EE_ICON_CUBE" Box##%d", i );
                    }
                    break;

                    case HitboxShape::Type::Sphere:
                    {
                        itemLabel.sprintf( EE_ICON_SPHERE" Sphere##%d", i );
                    }
                    break;
                }

                ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::MediumBoldItalic : ImGuiX::Font::Medium, HitboxShape::GetSeverityColor( shape.m_severity ) );
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
            //-------------------------------------------------------------------------

            if ( ImGui::BeginPopupContextItem() )
            {
                UUID shapeID = shape.m_ID;

                if ( ImGui::BeginMenu( EE_ICON_ALERT_DECAGRAM" Severity" ) )
                {
                    auto SeverityItem = [this, shapeID] ( char const* pLabel, HitboxDamageSeverity severity )
                    {
                        if ( ImGui::MenuItem( pLabel ) )
                        {
                            auto pShape = GetShape( shapeID );
                            EE_ASSERT( pShape != nullptr );
                            ScopedDataFileModification sm( this );
                            pShape->m_severity = severity;
                        }
                    };

                    SeverityItem( "Low", HitboxDamageSeverity::Low );
                    SeverityItem( "Medium", HitboxDamageSeverity::Medium );
                    SeverityItem( "High", HitboxDamageSeverity::High );
                    SeverityItem( "Critical", HitboxDamageSeverity::Critical );

                    ImGui::EndMenu();
                }

                if ( ImGui::MenuItem( EE_ICON_DELETE" Delete" ) )
                {
                    auto DeleteShape = [this, shapeID] ()
                    {
                        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
                        for ( auto shapeIter = pHitboxDescriptor->m_shapes.begin(); shapeIter != pHitboxDescriptor->m_shapes.end(); ++shapeIter )
                        {
                            if ( shapeIter->m_ID == shapeID )
                            {
                                ScopedDataFileModification sm( this );
                                pHitboxDescriptor->m_shapes.erase( shapeIter );
                                m_selectedItemID.Clear();
                                return;
                            }
                        }
                    };

                    m_commandStack.PushCommand( DeleteShape );
                }

                ImGui::EndPopup();
            }

            // Selection
            //-------------------------------------------------------------------------

            if ( ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenBlockedByPopup ) )
            {
                if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) || ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
                {
                    m_selectedItemID = shape.m_ID;
                }
            }
        }
    }

    void HitboxEditor::DrawDetailsWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsDataFileLoaded() || !m_setupResource.IsLoaded() )
        {
            return;
        }
        //-------------------------------------------------------------------------

        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        IReflectedType* pTypeToEdit = GetSelectedShape();
        if ( m_propertyGrid.GetEditedType() != pTypeToEdit )
        {
            m_propertyGrid.SetTypeToEdit( pTypeToEdit );
        }

        //-------------------------------------------------------------------------

        m_propertyGrid.UpdateAndDraw();
    }

    void HitboxEditor::DrawPreviewControlsWindow( UpdateContext const& context, bool isFocused )
    {
        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        EE_ASSERT( pHitboxDescriptor != nullptr );

        //-------------------------------------------------------------------------

        if ( m_pMeshComponent != nullptr )
        {
            ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
            if ( ImGui::CollapsingHeader( "Visualization" ) )
            {
                bool isMeshVisible = m_pMeshComponent->IsVisible();
                if ( ImGui::Checkbox( "Show Mesh", &isMeshVisible ) )
                {
                    m_pMeshComponent->SetVisible( isMeshVisible );
                }

                ImGui::Checkbox( "Draw Socket Debug", &m_drawSocketTransforms );
            }
        }

        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !IsResourceLoaded() );
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
                ImGui::BeginDisabled( IsPreviewing() );
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
                ImGui::EndDisabled();

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

                ImGui::EndTable();
            }
        }

        ImGui::EndDisabled();
    }

    void HitboxEditor::DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        TResourceEditor<HitboxDefinition>::DrawViewportUI( context, pViewport, isFocused );

        if ( !IsDataFileLoaded() )
        {
            return;
        }

        if ( !m_setupResource.IsLoaded() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( IsPreviewing() )
        {
            DrawViewportPreviewUI( context, pViewport, isFocused );
        }
        else
        {
            DrawViewportEditingUI( context, pViewport, isFocused );
        }
    }

    void HitboxEditor::DrawShapeGizmo( Viewport const* pViewport, bool isFocused, HitboxShape* pSelectedShape )
    {
        auto TryGetGizmoTransform = [this] ( Transform &outTransform ) -> bool
        {
            outTransform = GetSelectedShapeTransform();
            return true;
        };

        auto ApplyGizmoResult = [this] ( ImGuiX::Gizmo::Result const& result )
        {
            if ( result.IsScale() )
            {
                ScaleSelectedShape( result.m_delta );
            }
            else
            {
                Transform shapeTransform = GetSelectedShapeTransform();
                shapeTransform = result.GetModifiedTransform( shapeTransform );
                SetSelectedShapeTransform( shapeTransform );
            }
        };

        //-------------------------------------------------------------------------

        Transform gizmoTransform;
        bool const wasAbleToGetGizmoTransform = TryGetGizmoTransform( gizmoTransform );

        if ( wasAbleToGetGizmoTransform )
        {
            m_gizmo.SetCoordinateSystemSpace( CoordinateSpace::Local );
            m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, true );
            m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowNonUniformScale, pSelectedShape->m_type != HitboxShape::Type::Sphere );

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

    void HitboxEditor::DrawViewportEditingUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        auto drawingCtx = GetDebugDrawContext();
        auto pHitboxDescriptor = GetDataFile<HitboxResourceDescriptor>();
        EE_ASSERT( pHitboxDescriptor != nullptr );

        bool applyIsolation = ( m_isolateSelected && IsShapeSelected() );

        // Draw skeleton/sockets
        //-------------------------------------------------------------------------

        if ( IsUsingMeshAsSetupResource() )
        {
            auto pMesh = m_setupResource.GetPtr<Render::SkeletalMesh>();
            pMesh->DrawBindPose( drawingCtx, Transform::Identity, m_isolateSelected ? false : true );
        }
        else if ( IsUsingSkeletonAsSetupResource() )
        {
            auto pSkeleton = m_setupResource.GetPtr<Animation::Skeleton>();
            Animation::DrawOptions options;
            options.m_drawBoneNames = applyIsolation ? false : true;

            pSkeleton->DrawDebug( drawingCtx, Transform::Identity, Animation::Skeleton::LOD::Low, options );
        }

        // Draw Shapes
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < (int32_t) pHitboxDescriptor->m_shapes.size(); i++ )
        {
            auto const& shape = pHitboxDescriptor->m_shapes[i];
            Transform socketTransform = GetSocketTransform( shape.m_socketID );
            Transform const shapeTransform = shape.m_offset * socketTransform;

            bool const isSelected = ( shape.m_ID == m_selectedItemID );
            Color const shapeColor = HitboxShape::GetSeverityColor( shape.m_severity ).GetAlphaVersion( applyIsolation ? ( isSelected ? 0.75f : 0.1f ) : 0.75f );

            drawingCtx.SetHitTestID( i );
            switch ( shape.m_type )
            {
                case HitboxShape::Type::Box:
                {
                    drawingCtx.DrawWireBox( shapeTransform, shape.m_boxExtents, shapeColor );
                }
                break;

                case HitboxShape::Type::Sphere:
                {
                    drawingCtx.DrawWireSphere( shapeTransform.GetTranslation(), shape.m_radius, shapeColor );
                }
                break;

                case HitboxShape::Type::Capsule:
                {
                    drawingCtx.DrawWireCapsule( shapeTransform, shape.m_radius, shape.m_halfHeight, shapeColor );
                }
                break;
            }
            drawingCtx.ClearHitTestID();
        }

        // Gizmo
        //-------------------------------------------------------------------------

        if ( m_selectedItemID.IsValid() )
        {
            HitboxShape* pSelectedShape = GetSelectedShape();
            if ( pSelectedShape != nullptr )
            {
                DrawShapeGizmo( pViewport, isFocused, pSelectedShape );
            }
            else
            {
                m_gizmo.Reset();
            }
        }

        // Selection
        //-------------------------------------------------------------------------

        if ( !m_gizmo.IsManipulating() && m_isViewportHovered && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
            auto const& pd = GetPickingData();
            PickingID const pickingID = GetPickingID();
            if ( pickingID.IsSet() && !pickingID.HasSecondaryID() )
            {
                m_selectedItemID = pHitboxDescriptor->m_shapes[pickingID.m_primaryID].m_ID;
            }
            else
            {
                m_selectedItemID.Clear();
            }
        }
    }

    void HitboxEditor::DrawViewportPreviewUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        EE_ASSERT( IsPreviewing() && m_pHitbox != nullptr );

        auto drawingCtx = GetDebugDrawContext();
        m_pHitbox->DrawDebug( drawingCtx, m_drawSocketTransforms );
    }
}