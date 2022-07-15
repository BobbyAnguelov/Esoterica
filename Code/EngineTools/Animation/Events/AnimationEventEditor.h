#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/TimelineEditor/TimelineEditor.h"
#include "System/FileSystem/FileSystemPath.h"


//-------------------------------------------------------------------------

namespace EE { class UpdateContext; }
namespace EE::TypeSystem { class TypeRegistry; }
namespace EE::Animation { class AnimationClipPlayerComponent; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINETOOLS_API EventEditor final : public Timeline::TimelineEditor
    {
    public:

        EventEditor( TypeSystem::TypeRegistry const& typeRegistry );

        void Reset();
        void SetAnimationLengthAndFPS( uint32_t numFrames, float FPS );
        void UpdateAndDraw( UpdateContext const& context, AnimationClipPlayerComponent* pPreviewAnimationComponent );

        virtual bool Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& objectValue ) override;

    private:

        virtual void DrawAddTracksMenu() override;

        void UpdateTimelineTrackFPS();

    private:

        FileSystem::Path const                      m_descriptorPath;
        float                                       m_FPS = 0.0f;
        TVector<TypeSystem::TypeInfo const*>        m_eventTypes;
    };
}