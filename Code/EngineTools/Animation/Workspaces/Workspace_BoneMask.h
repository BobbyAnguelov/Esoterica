#pragma once

#include "EngineTools/Core/Workspace.h"
#include "EngineTools/Core/Helpers/SkeletonHelpers.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "System/Imgui/ImguiX.h"
#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class BoneMaskWorkspace final : public TWorkspace<BoneMaskDefinition>
    {
    public:

        using TWorkspace::TWorkspace;
        virtual ~BoneMaskWorkspace();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_HUMAN_EDIT; }

    private:

        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;
        virtual void OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded ) override;
        virtual void OnHotReloadComplete() override;

        void LoadSkeleton();
        void UnloadSkeleton();

        void CreateSkeletonTree();
        void DestroySkeletonTree();

        void CreateDescriptorWeights();

        void DrawWeightEditorWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        void DrawBoneWeightEditor( BoneInfo* pBone );
        void DrawMaskPreview();

        void SetWeight( int32_t boneIdx, float weight );
        void SetAllChildWeights( int32_t parentBoneIdx, float weight, bool bIncludeParent = false );
        void UpdateDemoBoneMask();

    private:

        String                                  m_weightEditorWindowName;
        BoneInfo*                               m_pSkeletonTreeRoot = nullptr;
        TVector<float>                          m_workingSetWeights;
        BoneMask                                m_demoBoneMask;
        TResourcePtr<Animation::Skeleton>       m_skeleton;
    };
}