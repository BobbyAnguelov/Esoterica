#pragma once

#include "EngineTools/Animation/Events/AnimationEventTimeline.h"
#include "EngineTools/Core/Timeline/TimelineEditor.h"
#include "EngineTools/Core/PropertyGrid/PropertyGrid.h"
#include "EngineTools/Core/EditorTool.h"
#include "Engine/Animation/AnimationClip.h"
#include "Base/Time/Timers.h"
#include "EngineTools/Animation/Shared/AnimationClipBrowser.h"

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

    class AnimationClipEditor : public TResourceEditor<AnimationClip>
    {
        EE_EDITOR_TOOL( AnimationClipEditor );

    public:

        AnimationClipEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID );
        ~AnimationClipEditor();

    private:

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;
        virtual void InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const override;

        virtual void PreWorldUpdate( EntityWorldUpdateContext const& updateContext ) override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;

        virtual void DrawMenu( UpdateContext const& context ) override;
        virtual void DrawHelpMenu() const override;

        virtual bool HasViewportToolbarTimeControls() const override { return true; }
        virtual void DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport ) override;

        virtual bool SaveData() override;

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

        virtual void OnDescriptorUnload() override;
        virtual void OnDescriptorLoadCompleted() override;
        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;
        virtual void OnResourceUnload( Resource::ResourcePtr* pResourcePtr ) override;

    private:

        Entity*                         m_pPreviewEntity = nullptr;
        AnimationClipPlayerComponent*   m_pAnimationComponent = nullptr;
        Render::SkeletalMeshComponent*  m_pMeshComponent = nullptr;

        EventTimeline                   m_eventTimeline;
        Timeline::TimelineEditor        m_timelineEditor;

        PropertyGrid                    m_propertyGrid;
        EventBindingID                  m_propertyGridPreEditEventBindingID;
        EventBindingID                  m_propertyGridPostEditEventBindingID;

        AnimationClipBrowser            m_animationClipBrowser;

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
        Skeleton::LOD                   m_skeletonLOD = Skeleton::LOD::High;

        TVector<StringID>               m_secondarySkeletonAttachmentSocketIDs;
    };
}