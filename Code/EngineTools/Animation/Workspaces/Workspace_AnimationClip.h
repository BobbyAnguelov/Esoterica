#pragma once

#include "EngineTools/Animation/Events/AnimationEventTrack.h"
#include "EngineTools/Core/Timeline/TimelineEditor.h"
#include "EngineTools/Core/PropertyGrid/PropertyGrid.h"
#include "EngineTools/Core/Workspace.h"
#include "Engine/Animation/AnimationClip.h"
#include "Base/Time/Timers.h"

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

    class AnimationClipWorkspace : public TWorkspace<AnimationClip>
    {
    public:

        AnimationClipWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        ~AnimationClipWorkspace();

    private:

        virtual char const* GetDockingUniqueTypeName() const override { return "Animation Clip"; }
        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded ) override;
        virtual void OnHotReloadComplete() override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;

        virtual void PreWorldUpdate( EntityWorldUpdateContext const& updateContext ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        virtual void DrawMenu( UpdateContext const& context ) override;

        virtual bool HasViewportToolbarTimeControls() const override { return true; }
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport ) override;

        virtual bool Save() override;

        virtual void ReadCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& descriptorObjectValue ) override;
        virtual void WriteCustomDescriptorData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) override;
        virtual void PreUndoRedo( UndoStack::Operation operation ) override;

        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { EE_ASSERT( HasTitlebarIcon() ); return EE_ICON_RUN_FAST; }

        void DrawTimelineWindow( UpdateContext const& context, bool isFocused );
        void DrawTrackDataWindow( UpdateContext const& context, bool isFocused );
        void DrawDetailsWindow( UpdateContext const& context, bool isFocused );
        void DrawClipBrowser( UpdateContext const& context, bool isFocused );

        void CreatePreviewEntity();
        void DestroyPreviewEntity();
        void CreatePreviewMeshComponent();
        void DestroyPreviewMeshComponent();

    private:

        Entity*                         m_pPreviewEntity = nullptr;
        AnimationClipPlayerComponent*   m_pAnimationComponent = nullptr;
        Render::SkeletalMeshComponent*  m_pMeshComponent = nullptr;

        EventTimeline                   m_eventTimeline;
        Timeline::TimelineEditor        m_timelineEditor;

        PropertyGrid                    m_propertyGrid;
        EventBindingID                  m_propertyGridPreEditEventBindingID;
        EventBindingID                  m_propertyGridPostEditEventBindingID;

        ImGuiX::FilterWidget            m_clipBrowserFilter;
        TVector<ResourceID>             m_clipsWithSameSkeleton;
        TVector<ResourceID>             m_filteredClips;
        Timer<PlatformClock>            m_clipBrowserCacheRefreshTimer;

        Transform                       m_characterTransform = Transform::Identity;
        ResourceID                      m_previewMeshOverride;
        Percentage                      m_currentAnimTime = 0.0f;
        bool                            m_isRootMotionEnabled = true;
        bool                            m_isPoseDrawingEnabled = true;
        bool                            m_shouldDrawBoneNames = false;
        bool                            m_characterPoseUpdateRequested = false;
        bool                            m_isPreviewCapsuleDrawingEnabled = false;
        float                           m_previewCapsuleHalfHeight = 0.65f;
        float                           m_previewCapsuleRadius = 0.35f;

        bool                            m_isDetailsWindowFocused = false;
    };
}