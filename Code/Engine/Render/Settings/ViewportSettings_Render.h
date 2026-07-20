#pragma once
#include "Engine/Viewport/ViewportSettings.h"
#include "Engine/_Module/API.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render
{
    // Has to match `typedef uint RendererDebugVisualizationMode` !
    enum class DebugVisualizationMode : int8_t
    {
        None,
        DirectLight,
        EnvironmentMapLight,
        Shadows,
        Albedo,
        Normals,
        Metalness,
        Roughness,
        AmbientOcclusion,
        LightComplexity,
        SSAO,
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API RenderViewportSettings : public IViewportSettings
    {
        EE_REFLECT_TYPE( RenderViewportSettings );

    public:

        virtual char const* GetFriendlyName() const override { return "Rendering"; }
        virtual void DrawMenu( EntityWorld* pWorld ) override;

    public:

        DebugVisualizationMode      m_visualizationMode = DebugVisualizationMode::None;
        bool                        m_showWireframe = false;
        bool                        m_showStaticMeshBounds = false;
        bool                        m_showSkeletalMeshBounds = false;
        bool                        m_showSkeletalMeshBones = false;
        bool                        m_showSkeletalMeshBindPoses = false;
        bool                        m_showShadowCascades = false;
    };
}
#endif