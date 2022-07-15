#pragma once

#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsRagdoll.h"
#include "EngineTools/Core/Workspaces/ResourceWorkspace.h"
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

    class RagdollWorkspace : public TResourceWorkspace<RagdollDefinition>
    {
        friend class ScopedRagdollSettingsModification;

        //-------------------------------------------------------------------------

        enum class Mode
        {
            BodyEditor,
            JointEditor
        };

    public:

        RagdollWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID, bool shouldLoadResource = true );
        virtual ~RagdollWorkspace();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void EndHotReload() override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context ) override;
        virtual bool HasViewportToolbar() const override { return true; }
        virtual bool HasViewportToolbarTimeControls() const override { return true; }
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;

        //-------------------------------------------------------------------------

        void LoadDefinition();
        void UnloadDefinition();

        EE_FORCE_INLINE RagdollDefinition* GetRagdollDefinition()
        {
            return &GetDescriptorAs<RagdollResourceDescriptor>()->m_definition;
        }

        // Definition Editing
        //-------------------------------------------------------------------------

        void DrawBodyEditorWindow( UpdateContext const& context );
        void DrawBodyEditorDetailsWindow( UpdateContext const& context );
        void CreateSkeletonTree();
        void DestroySkeletonTree();
        ImRect RenderSkeletonTree( BoneInfo* pBone );

        //-------------------------------------------------------------------------

        void DrawProfileEditorWindow( UpdateContext const& context );
        void DrawProfileManager();
        void DrawRootBodySettings( UpdateContext const& context, RagdollDefinition::Profile* pProfile );
        void DrawJointSettingsTable( UpdateContext const& context, RagdollDefinition::Profile* pProfile );
        void DrawMaterialSettingsTable( UpdateContext const& context, RagdollDefinition::Profile* pProfile );

        //-------------------------------------------------------------------------

        void CreateBody( StringID boneID );
        void DestroyBody( int32_t bodyIdx );
        void DestroyChildBodies( int32_t bodyIdx );

        void AutogenerateBodiesAndJoints();
        void AutogenerateBodyWeights();
        void CorrectInvalidSettings();

        void SetActiveProfile( StringID profileID );
        StringID CreateProfile();
        void DestroyProfile( StringID profileID );

        // Preview
        //-------------------------------------------------------------------------

        inline bool IsPreviewing() const { return m_pRagdoll != nullptr; }
        void StartPreview( UpdateContext const& context );
        void StopPreview();
        virtual void UpdateWorld( EntityWorldUpdateContext const& updateContext ) override;

        void DrawPreviewControlsWindow( UpdateContext const& context );

    private:

        String                                          m_bodyEditorWindowName;
        String                                          m_bodyEditorDetailsWindowName;
        String                                          m_profileEditorWindowName;
        String                                          m_previewControlsWindowName;

        // Data loaded with definition
        TResourcePtr<Animation::Skeleton>               m_pSkeleton; // This is kept separate since hot-reload can destroy the descriptor (and we can edit the descriptor)
        bool                                            m_definitionLoaded = false;
        

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
        StringID                                        m_originalPrenameID;
        char                                            m_profileNameBuffer[256];
        StringID                                        m_activeProfileID;
        RagdollDefinition::Profile                      m_workingProfileCopy; // Backing storage for imgui editing
        bool                                            m_drawSkeleton = true;
        bool                                            m_drawBodyAxes = false;
        bool                                            m_isolateSelectedBody = false;

        // Preview
        Resource::ResourceFilePicker                    m_resourceFilePicker;
        TResourcePtr<Animation::AnimationClip>          m_pPreviewAnimation;
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