#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/TimelineEditor/TimelineEditor.h"
#include "Base/FileSystem/FileSystemPath.h"


//-------------------------------------------------------------------------

namespace EE { class UpdateContext; }
namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINETOOLS_API EventEditor final : public Timeline::TimelineEditor
    {
    public:

        EventEditor( TypeSystem::TypeRegistry const& typeRegistry );

        void Reset();
        void SetAnimationInfo( uint32_t numFrames, float FPS );

        virtual bool Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& objectValue ) override;

    private:

        virtual void DrawAddTracksMenu() override;
        virtual float ConvertSecondsToTimelineUnit( Seconds const inTime ) const override { return inTime.ToFloat() * m_FPS; }

    private:

        FileSystem::Path const                      m_descriptorPath;
        float                                       m_FPS = 0.0f;
        TVector<TypeSystem::TypeInfo const*>        m_eventTrackTypes;
    };
}