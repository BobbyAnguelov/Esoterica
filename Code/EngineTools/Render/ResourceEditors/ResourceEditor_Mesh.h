#pragma once

#include "ResourceEditor_Mesh.h"
#include "EngineTools/Core/EditorTool.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------
// Static Mesh Editor
//-------------------------------------------------------------------------

namespace EE::Render
{
    class StaticMeshComponent;

    //-------------------------------------------------------------------------

    class StaticMeshEditor final : public TResourceEditor<StaticMesh>
    {
        EE_EDITOR_TOOL( StaticMeshEditor );

    public:

        using TResourceEditor::TResourceEditor;

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual void DrawHelpMenu() const override;
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_PINE_TREE; }
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;

        void DrawInfoWindow( UpdateContext const& context, bool isFocused );

    private:

        Entity*                         m_pPreviewEntity = nullptr;
        StaticMeshComponent*            m_pMeshComponent = nullptr;
        bool                            m_showOrigin = true;
        bool                            m_showBounds = true;
        bool                            m_showVertices = false;
        bool                            m_showNormals = false;
    };
}

//-------------------------------------------------------------------------
// Skeletal Mesh Editor
//-------------------------------------------------------------------------

namespace EE::Render
{
    class SkeletalMeshComponent;

    //-------------------------------------------------------------------------

    class SkeletalMeshEditor : public TResourceEditor<SkeletalMesh>
    {
        EE_EDITOR_TOOL( SkeletalMeshEditor );

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

    public:

        using TResourceEditor::TResourceEditor;
        virtual ~SkeletalMeshEditor();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;

        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_HUMAN; }

        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual void DrawHelpMenu() const override;

        void CreateSkeletonTree();
        void DestroySkeletonTree();
        ImRect RenderSkeletonTree( BoneInfo* pBone );

        void DrawSkeletonTreeWindow( UpdateContext const& context, bool isFocused );
        void DrawInfoWindow( UpdateContext const& context, bool isFocused );

    private:

        Entity*                             m_pPreviewEntity = nullptr;
        SkeletalMeshComponent*              m_pMeshComponent = nullptr;
        BoneInfo*                           m_pSkeletonTreeRoot = nullptr;
        StringID                            m_selectedBoneID;
        float                               m_skeletonViewPanelProportionalHeight = 0.75f;

        bool                                m_showOrigin = false;
        bool                                m_showBounds = true;
        bool                                m_showVertices = false;
        bool                                m_showNormals = false;
        bool                                m_showBindPose = true;
    };
}