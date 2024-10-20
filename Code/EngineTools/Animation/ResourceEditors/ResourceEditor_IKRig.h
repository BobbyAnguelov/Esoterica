#pragma once

#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Animation/Shared/AnimationClipBrowser.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_IKRig.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/IK/IKRig.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Imgui/ImguiGizmo.h"
#include "EngineTools/Widgets/Pickers.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class SkeletalMeshComponent;
}

namespace EE::Animation
{
    class IKRigEditor final : public TResourceEditor<IKRigDefinition>
    {
        EE_EDITOR_TOOL( IKRigEditor );

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

        enum class DebugState
        {
            None,
            RequestStart,
            Debugging
        };

        struct EffectorState
        {
            enum class Behavior
            {
                Fixed,
                Follow
            };

            int32_t                         m_linkIdx = InvalidIndex;
            int32_t                         m_boneIdx = InvalidIndex;
            Transform                       m_transform = Transform::Identity;
            Transform                       m_offsetTransform = Transform::Identity; // Only relevant when following
            Behavior                        m_behavior = Behavior::Follow;
        };

    public:

        IKRigEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld );
        virtual ~IKRigEditor();

    private:

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_HUMAN_EDIT; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawMenu( UpdateContext const& context ) override;

        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual void WorldUpdate( EntityWorldUpdateContext const& updateContext ) override;

        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        // Descriptor + Resources
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE IKRigResourceDescriptor* GetRigDescriptor()
        {
            return GetDataFile<IKRigResourceDescriptor>();
        }

        EE_FORCE_INLINE IKRigResourceDescriptor const* GetRigDescriptor() const
        {
            return GetDataFile<IKRigResourceDescriptor>();
        }

        virtual void OnDataFileUnload() override;
        virtual void OnDataFileLoadCompleted() override;
        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;

        // Skeleton
        //-------------------------------------------------------------------------

        void LoadSkeleton();
        void UnloadSkeleton();
        void OnSkeletonLoaded();

        // Rig Editing
        //-------------------------------------------------------------------------

        void DrawRigEditorWindow( UpdateContext const& context, bool isFocused );
        void DrawRigEditorDetailsWindow( UpdateContext const& context, bool isFocused );
        void CreateSkeletonTree();
        void DestroySkeletonTree();
        void RenderSkeletonTreeRow( BoneInfo* pBone, int32_t indentLevel = 0 );

        int32_t GetBodyIndexForBoneIdx( int32_t boneIdx ) const;
        int32_t GetBodyIndexForBoneID( StringID boneID ) const;
        bool IsParentBoneInRig( StringID boneID ) const;
        bool IsLeafBody( StringID boneID ) const;

        void AddBoneToRig( StringID boneID );
        void RemoveBoneFromRig( StringID boneID );

        // Debug
        //-------------------------------------------------------------------------

        void CreatePreviewEntity();
        void DestroyPreviewEntity();

        void DrawDebugControlsWindow( UpdateContext const& context, bool isFocused );

        inline bool IsDebugging() const { return m_pRig != nullptr; }
        void RequestDebugSession();
        void StartDebugging();
        void StopDebugging();

        void CreateEffectorDebugState();
        void DestroyEffectorGizmos();

    private:

        TResourcePtr<Animation::Skeleton>           m_skeleton;

        // Rig editor
        PropertyGrid                                m_propertyGrid;
        EventBindingID                              m_preEditEventBindingID;
        EventBindingID                              m_postEditEventBindingID;
        BoneInfo*                                   m_pSkeletonTreeRoot = nullptr;
        StringID                                    m_selectedBoneID;

        Entity*                                     m_pPreviewEntity = nullptr;
        Render::SkeletalMeshComponent*              m_pPreviewMeshComponent = nullptr;
        bool                                        m_showPreviewMesh = true;

        DebugState                                  m_debugState = DebugState::None;
        TVector<EffectorState>                      m_effectorDebugStates;
        ImGuiX::Gizmo                               m_effectorGizmo;
        ImGuiX::Gizmo                               m_offsetEffectorGizmo;
        int32_t                                     m_selectedEffectorIndex = InvalidIndex;

        Animation::Pose*                            m_pPreviewPose = nullptr;
        Pose*                                       m_pFinalPose = nullptr;
        IKRig*                                      m_pRig = nullptr;
        Vector                                      m_poseOffset = Vector( 0, 0.5f, 0.0f );
        bool                                        m_showPoseDebug = false;

        ResourcePicker                              m_resourceFilePicker;
        TResourcePtr<Animation::AnimationClip>      m_previewAnimation;
        Percentage                                  m_animTime = 0.0f;
        bool                                        m_isPlayingAnimation = false;
        bool                                        m_enableAnimationLooping = true;
        bool                                        m_autoPlayAnimation = true;
    };
}