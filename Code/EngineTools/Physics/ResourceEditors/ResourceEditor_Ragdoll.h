#pragma once

#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsRagdoll.h"
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Widgets/Pickers/ResourcePickers.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Physics/Ragdoll/PhysicsRagdoll_Instance.h"
#include "Engine/Imgui/ImguiGizmo.h"
#include "Base/Imgui/ImguiCommandStack.h"
#include "Base/Time/Timers.h"

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

        //-------------------------------------------------------------------------

        struct RagdollElement
        {
            enum class Type
            {
                None,
                BoneOrBody,
                Joint,
                Shape,
            };

        public:

            inline void Clear() { m_type = Type::None; m_boneID.Clear(); m_shapeIdx = InvalidIndex; }

            inline Type GetType() const { return m_type; }

            inline void SetBoneOrBody( StringID boneID ) { m_type = Type::BoneOrBody; m_boneID = boneID; m_shapeIdx = InvalidIndex; }
            inline void SetJoint( StringID boneID ) { m_type = Type::Joint; m_boneID = boneID; m_shapeIdx = InvalidIndex; }
            inline void SetShape( StringID boneID, int32_t shapeIdx ) { m_type = Type::Shape; m_boneID = boneID; m_shapeIdx = shapeIdx; }

            inline bool IsSet() const { return m_type != Type::None; }
            inline bool IsBoneOrBody() const { return m_type == Type::BoneOrBody; }
            inline bool IsJoint() const { return m_type == Type::Joint; }
            inline bool IsShape() const { return m_type == Type::Shape; }

        public:

            StringID        m_boneID;
            int32_t         m_shapeIdx = InvalidIndex;

        private:

            Type            m_type = Type::None;
        };

        //-------------------------------------------------------------------------

        constexpr static float const s_manipulationBodyRadius = 0.01f;

    public:

        RagdollEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld );
        virtual ~RagdollEditor();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual void WorldUpdate( EntityWorldUpdateContext const& updateContext ) override;
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_HUMAN_GREETING; }
        virtual bool SupportsToolbar() const override { return true; }
        virtual void DrawToolbar( UpdateContext const& context ) override;
        virtual bool SupportsWorldTimeControls() const override { return true; }
        virtual void ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport ) override;
        virtual bool ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport ) override;
        virtual void DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused ) override;
        virtual void HandleViewportInteractions() override;
        virtual bool IsDataFileManualEditingAllowed() const override { return false; }

        // Descriptor / Resource Management
        //-------------------------------------------------------------------------

        void LoadSkeleton();
        void UnloadSkeleton();
        void FixUpDescriptorBodiesAndSettings();

        // Shared
        //-------------------------------------------------------------------------

        Transform GetSelectedElementTransform();

        // Ragdoll Editing
        //-------------------------------------------------------------------------

        void DrawOutlinerWindow( UpdateContext const& context, bool isFocused );
        void DrawDetailsWindow( UpdateContext const& context, bool isFocused );
        void DrawSelfCollisionEditorWindow( UpdateContext const& context, bool isFocused );

        void CreateSkeletonTree();
        void DestroySkeletonTree();

        ImRect DrawOutlinerBoneItem( BoneInfo* pBone );
        void DrawOutlinerBodyItems( int32_t bodyIdx );

        void CreateBody( StringID boneID );
        void DestroyBody( int32_t bodyIdx );
        void DestroyChildBodies( int32_t bodyIdx );

        void CreateShape( int32_t bodyIdx );
        bool CanDestroyShape( int32_t bodyIdx, int32_t shapeIdx ) const;
        void DestroyShape( int32_t bodyIdx, int32_t shapeIdx );

        void CopyShapeCollisionSettings( int32_t bodyIdx, int32_t shapeIdx );

        void UpdateJointSetupDueToHierarchyChange();
        void ResetJointTransform( int32_t bodyIdx );
        void ChangeJointType( int32_t bodyIdx, RagdollJointDefinition::Type jointType );

        void DrawViewportUIForEditing( UpdateContext const& context, Viewport const* pViewport, bool isFocused );

        uint64_t ConvertRagdollElementToHitTestID( RagdollElement const& element ) const;
        RagdollElement ConvertHitTestIDToRagdollElement( uint64_t hitTestID ) const;

        // Will return null for invalid selection
        IReflectedType* GetTypeToEditBasedOnSelection() const;

        // Preview
        //-------------------------------------------------------------------------

        void CreatePreviewEntity();
        void DestroyPreviewEntity();

        void CalculateInputPose( Seconds deltaTime );

        void DrawPreviewControlsWindow( UpdateContext const& context, bool isFocused );

        virtual void OnPreviewStarted() override;
        virtual void OnPreviewStopped() override;

        void DrawViewportUIForPreview( UpdateContext const& context, Viewport const* pViewport, bool isFocused );

        void CreateRagdollManipulationBody( b3RayResult const& result );
        void DestroyRagdollManipulationBody();
        void UpdateRagdollManipulationBody( UpdateContext const& context, Viewport const* pViewport, bool isFocused );

        // Hot-Reload
        //-------------------------------------------------------------------------

        virtual void OnDataFileUnload() override;
        virtual void OnDataFileLoadCompleted() override;
        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;

    private:

        TResourcePtr<Animation::Skeleton>               m_skeleton;

        // Body/Joint editor
        ImGuiX::Gizmo                                   m_gizmo;
        BoneInfo*                                       m_pSkeletonTreeRoot = nullptr;
        bool                                            m_drawSkeleton = true;
        bool                                            m_drawBodyAxes = false;
        bool                                            m_showMesh = true;
        bool                                            m_isolateSelectedBody = false;

        RagdollElement                                  m_selectedElement;

        PropertyGrid                                    m_propertyGrid;
        EventBindingID                                  m_preEditEventBindingID;
        EventBindingID                                  m_postEditEventBindingID;

        ImGuiX::CommandStack                            m_commandStack;
        ImGuiX::FilterWidget                            m_selfCollisionFilter;

        // Preview
        ResourcePicker                                  m_animPicker;
        TResourcePtr<Animation::AnimationClip>          m_previewAnimation;
        Animation::Pose*                                m_pInputPose = nullptr;
        Animation::Pose*                                m_pFinalPose = nullptr;
        Percentage                                      m_animTime = 0.0f;
        Transform                                       m_previewStartTransform = Transform::Identity;
        bool                                            m_enablePoseFollowing = true;
        float                                           m_gravityScale = 1.0f;
        bool                                            m_isPlayingAnimation = false;
        bool                                            m_enableAnimationLooping = true;
        bool                                            m_autoPlayAnimation = true;
        bool                                            m_drawInputAnimationPose = false;
        bool                                            m_drawFinalAnimationPose = false;
        bool                                            m_drawRagdoll = true;
        bool                                            m_applyRootMotion = false;
        float                                           m_physicsBlendWeight = 1.0f;
        Ragdoll*                                        m_pRagdoll = nullptr;
        Entity*                                         m_pPreviewEntity = nullptr;
        Render::SkeletalMeshComponent*                  m_pMeshComponent = nullptr;

        float                                           m_impulseStrength = 10.0f;
        b3BodyId                                        m_manipulationBodyID = {};
        b3JointId                                       m_manipulationJointID = {};
        Vector                                          m_manipulationPlanePoint = Vector::Zero;
        bool                                            m_isManipulatingRagdoll = false;
    };
}