#pragma once

#include "EngineTools/Core/Workspace.h"
#include "EngineTools/Core/Helpers/SkeletonHelpers.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class SkeletalMeshComponent;
}

namespace EE::Animation
{
    class SkeletonWorkspace final : public TWorkspace<Skeleton>
    {
    public:

        using TWorkspace::TWorkspace;
        virtual ~SkeletonWorkspace();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded ) override;
        virtual void OnHotReloadComplete() override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;

        void DrawDetailsWindow( UpdateContext const& context );
        void DrawSkeletonHierarchyWindow( UpdateContext const& context );

        void CreateSkeletonTree();
        void DestroySkeletonTree();
        ImRect DrawBone( BoneInfo* pBone );

        void CreatePreviewEntity();

    private:

        String                          m_skeletonTreeWindowName;
        String                          m_detailsWindowName;

        BoneInfo*                       m_pSkeletonTreeRoot = nullptr;
        StringID                        m_selectedBoneID;

        Entity*                         m_pPreviewEntity = nullptr;
        bool                            m_poseReset = true;
    };
}