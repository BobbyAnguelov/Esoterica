#pragma once

#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsRagdoll.h"
#include "EngineTools/Core/Workspace.h"
#include "EngineTools/Core/Helpers/SkeletonHelpers.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Physics/PhysicsRagdoll.h"
#include "Engine/ToolsUI/Gizmo.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render { class SkeletalMeshComponent; }

//-------------------------------------------------------------------------

namespace EE::Physics
{
    class PhysicsSystem;

    //-------------------------------------------------------------------------

    class RagdollWorkspace : public TWorkspace<RagdollDefinition>
    {
        friend class ScopedRagdollSettingsModification;

        //-------------------------------------------------------------------------

        enum class Mode
        {
            BodyEditor,
            JointEditor
        };

        enum class Operation
        {
            None,
            CreateProfile,
            RenameProfile,
        };

    public:

        RagdollWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID, bool shouldLoadResource = true );
        virtual ~RagdollWorkspace();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded ) override;
        virtual void OnHotReloadComplete() override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void PreUpdateWorld( EntityWorldUpdateContext const& updateContext ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context ) override;
        virtual bool HasViewportToolbar() const override { return true; }
        virtual bool HasViewportToolbarTimeControls() const override { return true; }
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;

        void DrawDialogs();

        // Resource Management
        //-------------------------------------------------------------------------

        void CreatePreviewEntity();
        void DestroyPreviewEntity();

        void LoadSkeleton();
        void UnloadSkeleton();

        EE_FORCE_INLINE RagdollResourceDescriptor* GetRagdollDescriptor()
        {
            return GetDescriptor<RagdollResourceDescriptor>();
        }

        EE_FORCE_INLINE RagdollDefinition* GetRagdollDefinition()
        {
            EE_ASSERT( m_pDescriptor != nullptr );
            return &GetDescriptor<RagdollResourceDescriptor>()->m_definition;
        }

        EE_FORCE_INLINE RagdollDefinition const* GetRagdollDefinition() const
        {
            EE_ASSERT( m_pDescriptor != nullptr );
            return &GetDescriptor<RagdollResourceDescriptor>()->m_definition;
        }

        // This will check the ragdoll definition and ensure that it is valid!
        void ValidateDefinition();

        // Body Editing
        //-------------------------------------------------------------------------

        void DrawBodyEditorWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        void DrawBodyEditorDetailsWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
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

        void DrawProfileEditorWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        void DrawProfileManager();
        void DrawRootControlBodySettings( UpdateContext const& context, RagdollDefinition::Profile* pProfile );
        void DrawJointSettingsTable( UpdateContext const& context, RagdollDefinition::Profile* pProfile );
        void DrawMaterialSettingsTable( UpdateContext const& context, RagdollDefinition::Profile* pProfile );

        StringID CreateProfile( String const ID );
        void ResetProfile( RagdollDefinition::Profile& profile ) const;
        void DestroyProfile( StringID profileID );
        String GetUniqueProfileName( String const& desiredName ) const;

        // Preview
        //-------------------------------------------------------------------------

        void DrawPreviewControlsWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        inline bool IsPreviewing() const { return m_pRagdoll != nullptr; }
        void StartPreview( UpdateContext const& context );
        void StopPreview();

    private:

        String                                          m_bodyEditorWindowName;
        String                                          m_bodyEditorDetailsWindowName;
        String                                          m_profileEditorWindowName;
        String                                          m_previewControlsWindowName;

        TResourcePtr<Animation::Skeleton>               m_skeleton;
        bool                                            m_needToCreateEditorState = false;
        Operation                                       m_activeOperation = Operation::None;

        // Body/Joint editor
        Mode                                            m_editorMode;
        Transform                                       m_gizmoWorkingTransform;
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
        Resource::ResourcePicker                        m_resourceFilePicker;
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
        ImGuiX::Gizmo                                   m_previewGizmo;
        Transform                                       m_previewGizmoTransform;

        // PVD connection
        PhysicsSystem*                                  m_pPhysicsSystem = nullptr;
        bool                                            m_autoConnectToPVD = false;
    };
}