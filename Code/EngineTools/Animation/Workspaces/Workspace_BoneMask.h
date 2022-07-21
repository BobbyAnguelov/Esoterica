#pragma once

#include "EngineTools/Core/Workspaces/ResourceWorkspace.h"
#include "EngineTools/Core/Helpers/SkeletonHelpers.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "System/Imgui/ImguiX.h"
#include "Engine/Animation/AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class BoneMaskWorkspace final : public TResourceWorkspace<BoneMaskDefinition>
    {
    public:

        using TResourceWorkspace::TResourceWorkspace;
        virtual ~BoneMaskWorkspace();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;

    private:

        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;
        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void EndHotReload() override;

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

        TResourcePtr<Animation::Skeleton>       m_pSkeleton; // This is kept separate since hot-reload can destroy the descriptor (and we can edit the descriptor)
        bool                                    m_skeletonLoadingRequested = false;
    };
}