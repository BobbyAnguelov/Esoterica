#pragma once

#include "EngineTools/Core/Workspace.h"
#include "EngineTools/Core/Helpers/SkeletonHelpers.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class SkeletalMeshComponent;

    //-------------------------------------------------------------------------

    class SkeletalMeshWorkspace : public TWorkspace<SkeletalMesh>
    {

    public:

        using TWorkspace::TWorkspace;
        virtual ~SkeletalMeshWorkspace();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_HUMAN; }

        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context ) override;

        void CreateSkeletonTree();
        void DestroySkeletonTree();
        ImRect RenderSkeletonTree( BoneInfo* pBone );

        void DrawSkeletonTreeWindow( UpdateContext const& context );
        void DrawMeshInfoWindow( UpdateContext const& context );
        void DrawDetailsWindow( UpdateContext const& context );

    private:

        Entity*                 m_pPreviewEntity = nullptr;
        SkeletalMeshComponent*  m_pMeshComponent = nullptr;
        BoneInfo*               m_pSkeletonTreeRoot = nullptr;
        StringID                m_selectedBoneID;

        String                  m_skeletonTreeWindowName;
        String                  m_meshInfoWindowName;
        String                  m_detailsWindowName;

        bool                    m_showNormals = false;
        bool                    m_showVertices = false;
        bool                    m_showBindPose = true;
        bool                    m_showBounds = true;
    };
}