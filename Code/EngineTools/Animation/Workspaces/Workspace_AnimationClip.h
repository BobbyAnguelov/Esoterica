#pragma once

#include "EngineTools/Core/PropertyGrid/PropertyGrid.h"
#include "EngineTools/Core/Workspaces/ResourceWorkspace.h"
#include "EngineTools/Animation/Events/AnimationEventEditor.h"
#include "Engine/Animation/AnimationClip.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class SkeletalMeshComponent;
}

namespace EE::Animation
{
    class AnimationClipPlayerComponent;
    class EventEditor;

    //-------------------------------------------------------------------------

    class AnimationClipWorkspace : public TResourceWorkspace<AnimationClip>
    {
    public:

        AnimationClipWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        ~AnimationClipWorkspace();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded ) override;
        virtual void EndHotReload() override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID ) const override;
        virtual void UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused ) override;

        virtual bool HasViewportToolbarTimeControls() const override { return true; }
        virtual void DrawViewportToolbarItems( UpdateContext const& context, Render::Viewport const* pViewport ) override;
        virtual void DrawWorkspaceToolbarItems( UpdateContext const& context ) override;

        virtual bool IsDirty() const override;
        virtual bool Save() override;

        virtual void SerializeCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& descriptorObjectValue ) override;
        virtual void SerializeCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) override;

        void DrawTimelineWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        void DrawTrackDataWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );
        bool DrawDetailsWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

        void CreatePreviewEntity();
        void DestroyPreviewEntity();
        void CreatePreviewMeshComponent();
        void DestroyPreviewMeshComponent();

    private:

        String                          m_timelineWindowName;
        String                          m_detailsWindowName;
        String                          m_trackDataWindowName;

        Entity*                         m_pPreviewEntity = nullptr;
        AnimationClipPlayerComponent*   m_pAnimationComponent = nullptr;
        Render::SkeletalMeshComponent*  m_pMeshComponent = nullptr;
        EventEditor                     m_eventEditor;
        PropertyGrid                    m_propertyGrid;
        EventBindingID                  m_propertyGridPreEditEventBindingID;
        EventBindingID                  m_propertyGridPostEditEventBindingID;

        EventBindingID                  m_beginModEventID;
        EventBindingID                  m_endModEventID;

        Transform                       m_characterTransform = Transform::Identity;
        ResourceID                      m_previewMeshOverride;
        Percentage                      m_currentAnimTime = 0.0f;
        bool                            m_isRootMotionEnabled = true;
        bool                            m_isPoseDrawingEnabled = true;
    };
}