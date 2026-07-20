#pragma once

#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Animation/AnimationClipBrowser.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class SkeletalMeshComponent;
}

namespace EE::Animation
{
    class SkeletonEditor final : public TResourceEditor<Skeleton>
    {
        EE_EDITOR_TOOL( SkeletonEditor );

        struct BoneItem
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

            ResourceID                      m_skeletonID;
            StringID                        m_boneID;
            int32_t                         m_boneIdx = InvalidIndex;
            TInlineVector<BoneItem*, 5>     m_children;
            bool                            m_isExpanded = true;
            float                           m_weight = 1.0f;
        };

    public:

        SkeletonEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld );
        virtual ~SkeletonEditor();

    private:

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_SKULL; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport ) override;
        virtual bool ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport ) override;

        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        virtual void OnDataFileUnload() override;
        virtual void OnDataFileLoadCompleted() override;
        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;

        void CreatePreviewEntity();
        void DestroyPreviewEntity();

        // Skeleton
        //-------------------------------------------------------------------------

        void CreateSkeletonTree();
        void DestroySkeletonTree();

        void DrawSkeletonOutlinerWindow( UpdateContext const& context, bool isFocused );
        void DrawBoneInfoWindow( UpdateContext const& context, bool isFocused );
        void DrawSkeletonTreeRow( BoneItem* pBone );

        // LOD
        //-------------------------------------------------------------------------

        void ValidateLODSetup();
        Skeleton::LOD GetBoneLOD( StringID BoneID ) const;
        void SetBoneHierarchyLOD( StringID BoneID, Skeleton::LOD lod );

        // Bone Masks
        //-------------------------------------------------------------------------

        void DrawBoneMasksWindow( UpdateContext const& context, bool isFocused );

        void ValidateDescriptorBoneMaskDefinitions();

        bool IsEditingBoneMask() const { return m_editedMaskIdx != InvalidIndex; }
        void StartEditingMask( StringID maskID );
        void StopEditingMask();

        void CreateBoneMask();
        void DestroyBoneMask( StringID maskID );
        void RenameBoneMask( StringID oldID, StringID newID );

        void SetWeight( int32_t boneIdx, float weight );
        void SetAllChildWeights( int32_t parentBoneIdx, float weight, bool bIncludeParent = false );

        void GenerateUniqueBoneMaskName( int32_t boneMaskIdx );

        bool DrawDeleteBoneMaskDialog();
        bool DrawRenameBoneMaskDialog();

        void ReflectEditedWeights();

        // Clip Browser
        //-------------------------------------------------------------------------

        void DrawClipBrowser( UpdateContext const& context, bool isFocused );

    private:

        Entity*                         m_pPreviewEntity = nullptr;
        Render::SkeletalMeshComponent*  m_pPreviewMeshComponent = nullptr;
        bool                            m_showPreviewMesh = true;

        // Skeleton View
        TVector<BoneItem*>              m_bones;
        BoneItem*                       m_pSkeletonTreeRoot = nullptr;
        TVector<StringID>               m_selectedBoneIDs;

        // Bone Mask View
        int32_t                         m_editedMaskIdx = InvalidIndex;
        BoneMask                        m_previewBoneMask;
        StringID                        m_dialogMaskID;
        char                            m_renameBuffer[255] = { 0 };

        // Clip Browser
        AnimationClipBrowser            m_animationClipBrowser;
    };
}