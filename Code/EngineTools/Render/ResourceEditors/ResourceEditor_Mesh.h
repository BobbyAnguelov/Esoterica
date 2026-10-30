#pragma once

#include "EngineTools/Core/EditorTool.h"
#include "Engine/Render/RenderMesh.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Imgui/ImguiCommandStack.h"
#include "EngineTools/Gizmos/Gizmo.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class MeshComponent;
    class MeshSectionList;

    //-------------------------------------------------------------------------

    class MeshEditor final : public DataFileEditor
    {
        EE_EDITOR_TOOL( MeshEditor );

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

            int32_t                         m_boneIdx;
            TInlineVector<BoneItem*, 5>     m_children;
            bool                            m_isExpanded = true;
        };

    public:

        MeshEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld = nullptr );
        virtual ~MeshEditor();

    private:

        void ClearAllSelections();

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void OnDataFileLoadCompleted() override;
        virtual void OnDataFileUnload() override;

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return m_isViewingSkeletalMesh ? EE_ICON_HUMAN : EE_ICON_PINE_TREE; }
        virtual bool SupportsNewFileCreation() const { return false; }
        virtual bool IsEditingResourceDescriptor() const override final { return true; }
        virtual void DrawHelpMenu() const override;
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;

        virtual void ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport ) override;
        virtual bool ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport ) override;

        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        // Resource Status
        //-------------------------------------------------------------------------

        inline bool IsLoading() const { return m_editedResource.IsLoading(); }
        inline bool IsUnloaded() const { return m_editedResource.IsUnloaded(); }
        inline bool IsResourceLoaded() const { return m_editedResource.IsLoaded(); }
        inline bool IsWaitingForResource() const { return IsLoading() || IsUnloaded() || m_editedResource.IsUnloading(); }
        inline bool HasLoadingFailed() const { return m_editedResource.HasLoadingFailed(); }

        virtual void DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused ) override;

        virtual TInlineVector<ResourceID, 2> GetPreviewResourceIDs() const final { return { m_editedResource.GetResourceID() }; }

        // Get the loaded resource (only valid to call when actually previewing or during 'OnPreviewStarted'/'OnPreviewEnded')
        template<typename PRT>
        PRT const* GetPreviewResource() const { return DataFileEditor::GetPreviewResource<PRT>( m_editedResource.GetResourceID() ); }

        // Preview
        //-------------------------------------------------------------------------

        void CreatePreviewEntity();
        void DestroyPreviewEntity();

        // Skeletal Mesh
        //-------------------------------------------------------------------------

        void CreateSkeletonTree();
        void DestroySkeletonTree();

        // Sockets
        //-------------------------------------------------------------------------

        void AddSocket();
        void RemoveSocket( int32_t socketIdx );
        Transform GetSelectedSocketTransform() const;
        void SetSelectedSocketTransform( Transform const& transform );

        // Mesh Decals
        //-------------------------------------------------------------------------

        void AddMeshDecal( ResourceID const& materialResourceID );
        void RemoveMeshDecal( int32_t decalIdx );
        Transform GetSelectedMeshDecalTransform() const;
        void SetSelectedMeshDecalTransform( Transform const& transform );
        Float3 GetSelectedMeshDecalNonUniformScale() const;
        void SetSelectedMeshDecalNonUniformScale( Float3 const& nonUniformScale );

        // Materials
        //-------------------------------------------------------------------------

        /*void ImportMaterials();*/

        // Windows + Widgets
        //-------------------------------------------------------------------------

        void DrawMeshInfoWindow( UpdateContext const& context, bool isFocused );
        void DrawSubmeshesWindow( UpdateContext const& context, bool isFocused );
        void DrawSocketsWindow( UpdateContext const& context, bool isFocused );
        void DrawMeshDecalsWindow( UpdateContext const& context, bool isFocused );
        void DrawSkeletonWindow( UpdateContext const& context, bool isFocused );

        void DrawSkeletonTreeRow( BoneItem* pBone );
        void DrawSocketGizmo( Viewport const* pViewport, bool isFocused );
        void DrawMeshDecalGizmo( Viewport const* pViewport, bool isFocused );

    private:

        ImGuiX::CommandStack            m_commandStack;
        Resource::ResourcePtr           m_editedResource = nullptr;

        //-------------------------------------------------------------------------

        BoneItem*                       m_pSkeletonTreeRoot = nullptr;
        TVector<StringID>               m_selectedBoneIDs;

        //-------------------------------------------------------------------------

        PropertyGrid                    m_socketPropertyGrid;
        EventBindingID                  m_preEditSocketBindingID;
        EventBindingID                  m_postEditSocketBindingID;
        int32_t                         m_selectedSocketIdx = InvalidIndex;
        Gizmo                           m_socketGizmo;

        //-------------------------------------------------------------------------

        PropertyGrid                    m_decalPropertyGrid;
        EventBindingID                  m_preEditDecalBindingID;
        EventBindingID                  m_postEditDecalBindingID;
        int32_t                         m_selectedDecalIndex = InvalidIndex;
        Gizmo                           m_decalGizmo;
        bool                            m_isEditingMeshDecals = false;

        //-------------------------------------------------------------------------

        TVector<StringID>               m_uniqueMaterialNames;
        TVector<int16_t>                m_selectedSubmeshes;
        TVector<int16_t>                m_hiddenSubmeshes;
        TVector<ResourcePicker*>        m_materialPickers;

        String                          m_importMaterialsResultMessage = {};

        //-------------------------------------------------------------------------

        Entity*                         m_pPreviewEntity = nullptr;
        MeshComponent*                  m_pMeshComponent = nullptr;

        float                           m_cameraDistance = 0.0f;
        int32_t                         m_LOD = -1;

        bool                            m_isViewingSkeletalMesh = true;
        bool                            m_showOrigin = true;
        bool                            m_showBounds = true;
        bool                            m_showVertices = false;
        bool                            m_showNormals = false;
        bool                            m_showBindPose = true;
    };
}