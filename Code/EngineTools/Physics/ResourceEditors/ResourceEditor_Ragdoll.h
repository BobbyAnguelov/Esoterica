#pragma once

#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsRagdoll.h"
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Widgets/Pickers.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "Base/Imgui/ImguiGizmo.h"

//-------------------------------------------------------------------------

namespace EE::Render { class SkeletalMeshComponent; }

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class RagdollEditor : public TResourceEditor<RagdollDefinition>
    {
        EE_EDITOR_TOOL( RagdollEditor );

        friend class ScopedRagdollSettingsModification;

        //-------------------------------------------------------------------------

        struct BoneInfo
        {
            inline void DestroyChildren()
            {
                for ( auto& pChild : m_children )
                {
                    pChild->DestroyChildren();
                    EE::Delete( pChild );
                }

                m_children.clear();
            }

        public:

            int32_t                         m_boneIdx;
            TInlineVector<BoneInfo*, 5>     m_children;
            bool                            m_isExpanded = true;
        };

        enum class Mode
        {
            BodyEditor,
            JointEditor
        };

        struct CollisionActor
        {
            physx::PxRigidDynamic*  m_pActor;
            float                   m_radius;
            float                   m_TTL;
        };

    public:

        RagdollEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld );
        virtual ~RagdollEditor();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual bool SaveData() override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual void WorldUpdate( EntityWorldUpdateContext const& updateContext ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_HUMAN_GREETING; }

        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual bool HasViewportToolbarTimeControls() const override { return true; }
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;

        // Resource Management
        //-------------------------------------------------------------------------

        void CreatePreviewEntity();
        void DestroyPreviewEntity();

        void LoadSkeleton();
        void UnloadSkeleton();

        EE_FORCE_INLINE RagdollResourceDescriptor* GetRagdollDescriptor()
        {
            return GetDataFile<RagdollResourceDescriptor>();
        }

        EE_FORCE_INLINE RagdollResourceDescriptor const* GetRagdollDescriptor() const
        {
            return GetDataFile<RagdollResourceDescriptor>();
        }

        bool IsValidDefinition() const { return GetRagdollDescriptor()->IsValid(); }

        // This will check the ragdoll definition and ensure that it is valid!
        void PerformAdvancedDefinitionValidation();

        // Body Editing
        //-------------------------------------------------------------------------

        void DrawBodyEditorWindow( UpdateContext const& context, bool isFocused );
        void DrawBodyEditorDetailsWindow( UpdateContext const& context, bool isFocused );
        void CreateSkeletonTree();
        void DestroySkeletonTree();
        ImRect RenderSkeletonTree( BoneInfo* pBone );

        void CreateBody( StringID boneID );
        void DestroyBody( int32_t bodyIdx );
        void DestroyChildBodies( int32_t bodyIdx );

        void AutogenerateBodiesAndJoints();
        void AutogenerateBodyWeights();

        // Profile Editing
        //-------------------------------------------------------------------------

        // Switches to a given profile as well as creating the backing store for the profile editing
        void SetActiveProfile( StringID profileID );

        void UpdateProfileWorkingCopy();

        void DrawProfileEditorWindow( UpdateContext const& context, bool isFocused );
        void DrawProfileManager();
        void DrawBodyAndJointSettingsTable( UpdateContext const& context, RagdollDefinition::Profile* pProfile );
        void DrawMaterialSettingsTable( UpdateContext const& context, RagdollDefinition::Profile* pProfile );

        StringID CreateProfile( String const newProfileName );
        StringID DuplicateProfile( StringID originalProfileID, String duplicateProfileName );
        void ResetProfile( RagdollDefinition::Profile& profile ) const;
        void DestroyProfile( StringID profileID );
        String GetUniqueProfileName( String const& desiredName ) const;

        bool DrawCreateProfileDialog( UpdateContext const& context );
        bool DrawDuplicateProfileDialog( UpdateContext const& context );
        bool DrawRenameProfileDialog( UpdateContext const& context );

        // Preview
        //-------------------------------------------------------------------------

        void DrawPreviewControlsWindow( UpdateContext const& context, bool isFocused );
        inline bool IsPreviewing() const { return m_pRagdoll != nullptr; }
        void StartPreview( UpdateContext const& context );
        void StopPreview();

        void SpawnCollisionActor( Vector const& startPos, Vector const& initialVelocity );
        void UpdateSpawnedCollisionActors( Drawing::DrawContext& drawingContext, Seconds deltaTime );
        void DestroySpawnedCollisionActors();

        // Hot-Reload
        //-------------------------------------------------------------------------

        virtual void OnDataFileUnload() override;
        virtual void OnDataFileLoadCompleted() override;
        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;

    private:

        TResourcePtr<Animation::Skeleton>               m_skeleton;
        RagdollDefinition                               m_ragdollDefinition;

        // Body/Joint editor
        ImGuiX::Gizmo                                   m_gizmo;
        Mode                                            m_editorMode;
        PropertyGrid                                    m_bodyEditorPropertyGrid;
        EventBindingID                                  m_bodyGridPreEditEventBindingID;
        EventBindingID                                  m_bodyGridPostEditEventBindingID;
        PropertyGrid                                    m_solverSettingsGrid;
        EventBindingID                                  m_solverGridPreEditEventBindingID;
        EventBindingID                                  m_solverGridPostEditEventBindingID;
        BoneInfo*                                       m_pSkeletonTreeRoot = nullptr;
        StringID                                        m_selectedBoneID;
        char                                            m_profileNameBuffer[256];
        StringID                                        m_activeProfileID;
        RagdollDefinition::Profile                      m_workingProfileCopy; // Backing storage for imgui editing
        bool                                            m_drawSkeleton = true;
        bool                                            m_drawBodyAxes = false;
        bool                                            m_isolateSelectedBody = false;

        // Preview
        ResourcePicker                                  m_resourceFilePicker;
        TResourcePtr<Animation::AnimationClip>          m_previewAnimation;
        Animation::Pose*                                m_pPose = nullptr;
        Animation::Pose*                                m_pFinalPose = nullptr;
        Percentage                                      m_animTime = 0.0f;
        Transform                                       m_previewStartTransform = Transform::Identity;
        bool                                            m_enablePoseFollowing = true;
        bool                                            m_enableGravity = true;
        bool                                            m_isPlayingAnimation = false;
        bool                                            m_enableAnimationLooping = true;
        bool                                            m_autoPlayAnimation = true;
        bool                                            m_drawAnimationPose = false;
        bool                                            m_drawRagdoll = true;
        bool                                            m_initializeRagdollPose = false;
        bool                                            m_applyRootMotion = false;
        bool                                            m_refreshRagdollSettings = false;
        float                                           m_physicsBlendWeight = 1.0f;
        Ragdoll*                                        m_pRagdoll = nullptr;
        Entity*                                         m_pPreviewEntity = nullptr;
        Render::SkeletalMeshComponent*                  m_pMeshComponent = nullptr;
        float                                           m_impulseStrength = 10.0f;
        float                                           m_collisionActorRadius = 0.25f;
        float                                           m_collisionActorMass = 50.0f;
        float                                           m_collisionActorLifetime = 10.0f;
        float                                           m_collisionActorInitialVelocity = 20.0f;
        bool                                            m_collisionActorGravity = true;
        TVector<CollisionActor>                         m_spawnedCollisionActors;
    };
}