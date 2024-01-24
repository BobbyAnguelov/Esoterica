#pragma once
#include "Engine/Entity/EntityWorldSettings.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    #if EE_DEVELOPMENT_TOOLS
    enum class DebugVisualizationMode : int8_t
    {
        Lighting = 0,
        Albedo = 1,
        Normals = 2,
        Metalness = 3,
        Roughness = 4,
        AmbientOcclusion = 5,

        BitShift = 32 - 3,
    };
    #endif

    //-------------------------------------------------------------------------

    class RenderWorldSettings : public IEntityWorldSettings
    {
        EE_REFLECT_TYPE( RenderWorldSettings );

    public:

        #if EE_DEVELOPMENT_TOOLS
        DebugVisualizationMode                   m_visualizationMode = DebugVisualizationMode::Lighting;
        bool                                m_showStaticMeshBounds = false;
        bool                                m_showSkeletalMeshBounds = false;
        bool                                m_showSkeletalMeshBones = false;
        bool                                m_showSkeletalMeshBindPoses = false;
        #endif
    };
}