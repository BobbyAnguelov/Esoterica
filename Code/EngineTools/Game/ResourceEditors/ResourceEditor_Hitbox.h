#pragma once

#include "EngineTools/Game/ResourceDescriptors/ResourceDescriptor_Hitbox.h"
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Widgets/Pickers/ResourcePickers.h"
#include "Engine/Hitbox/Hitbox_Definition.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Render/RenderMesh.h"
#include "Engine/Imgui/ImguiGizmo.h"
#include "Base/Imgui/ImguiCommandStack.h"
#include "Base/Time/Timers.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Render { class SkeletalMeshComponent; }

//-------------------------------------------------------------------------

namespace EE
{
    class HitboxEditor final : public TResourceEditor<HitboxDefinition>
    {
        EE_EDITOR_TOOL( HitboxEditor );

        struct SocketInfo
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

            UUID                            m_ID = UUID::GenerateID();
            StringID                        m_boneID;
            int32_t                         m_boneIdx;
            TInlineVector<SocketInfo*, 5>   m_children;
            bool                            m_isExpanded = true;
        };

    public:

        using TResourceEditor::TResourceEditor;

        HitboxEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld );
        virtual ~HitboxEditor();

    private:

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_PALETTE_SWATCH; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;
        virtual bool HasViewportToolbar() const { return true; }
        virtual bool SupportsToolbar() const override { return true; }
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual bool IsDataFileManualEditingAllowed() const { return false; }
        virtual void PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction ) override;

        // UI
        //-------------------------------------------------------------------------

        virtual void DrawToolbar( UpdateContext const& context ) override;
        void DrawShapeEditorWindow( UpdateContext const& context, bool isFocused );
        void DrawDetailsWindow( UpdateContext const& context, bool isFocused );
        void DrawPreviewControlsWindow( UpdateContext const& context, bool isFocused );

        virtual void DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused ) override;
        void DrawViewportEditingUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused );
        void DrawViewportPreviewUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused );
        ImRect DrawSocketTreeItem( SocketInfo* pSocket );
        void DrawShapesForSocket( int32_t socketIndex );
        void DrawShapeGizmo( Viewport const* pViewport, bool isFocused, HitboxShape* pSelectedShape );

        // Editing
        //-------------------------------------------------------------------------

        bool IsUsingSkeletonAsSetupResource() const;
        bool IsUsingMeshAsSetupResource() const;
        int32_t GetSocketIndex( StringID socketID ) const;

        void CreateSocketTree();
        void DestroySocketTree();

        bool IsShapeSelected() const;

        HitboxShape* GetShape( UUID const& ID );
        HitboxShape* GetSelectedShape() { return GetShape( m_selectedItemID ); }

        Transform GetSelectedElementTransform() const;
        Transform GetSocketTransform( StringID socketID ) const;
        Transform GetShapeTransform( UUID const& ID ) const;
        Transform GetSelectedShapeTransform() const { return GetShapeTransform( m_selectedItemID ); }
        void SetShapeTransform( UUID const& ID, Transform const& transform );
        void SetSelectedShapeTransform( Transform const& transform ) { SetShapeTransform( m_selectedItemID, transform ); }
        void ScaleShape( UUID const& ID, Vector const& scaleDelta );
        void ScaleSelectedShape( Vector scaleDelta ) { ScaleShape( m_selectedItemID, scaleDelta ); }

        // Get the transforms needed to create the initial shape size/offset, returns true if a valid child transform was found
        bool GetShapeCreationTransforms( StringID socketID, Transform& socketTransform, Transform& socketChildTransform );

        void CreateBoxShape( StringID socketID );
        void CreateSphereShape( StringID socketID );
        void CreateCapsuleShape( StringID socketID );

        // Preview
        //-------------------------------------------------------------------------

        void CreatePreviewEntity( bool setSkeleton = false );
        void DestroyPreviewEntity();

        virtual void OnPreviewStarted() override;
        virtual void OnPreviewStopped() override;

        void SetAndLoadSetupResource();
        void UnloadAndClearSetupResource();

        void CalculatePreviewPose( Seconds deltaTime );

        // Hot-Reload
        //-------------------------------------------------------------------------

        virtual void OnDataFileUnload() override;
        virtual void OnDataFileLoadCompleted() override;
        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;

    private:

        PropertyGrid                                    m_propertyGrid;
        EventBindingID                                  m_preEditEventBindingID;
        EventBindingID                                  m_postEditEventBindingID;

        ImGuiX::CommandStack                            m_commandStack;
        Resource::ResourcePtr                           m_setupResource; // The resource that we use using to set up the shapes

        SocketInfo*                                     m_pSocketTreeRoot = nullptr;

        ResourcePicker                                  m_setupResourcePicker;
        ImGuiX::FilterWidget                            m_socketFilter;
        UUID                                            m_selectedItemID;
        bool                                            m_isolateSelected = true;

        Entity*                                         m_pPreviewEntity = nullptr;
        Render::SkeletalMeshComponent*                  m_pMeshComponent = nullptr;

        Hitbox*                                         m_pHitbox = nullptr;
        ResourcePicker                                  m_animPicker;
        TResourcePtr<Animation::AnimationClip>          m_previewAnimation;
        Animation::Pose*                                m_pPose = nullptr;
        Percentage                                      m_animTime = 0.0f;
        bool                                            m_isPlayingAnimation = false;
        bool                                            m_autoPlayAnimation = true;
        bool                                            m_enableAnimationLooping = true;
        bool                                            m_drawSocketTransforms = false;

        ImGuiX::Gizmo                                   m_gizmo;
    };
}