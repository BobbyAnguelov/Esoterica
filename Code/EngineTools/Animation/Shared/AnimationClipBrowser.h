#pragma once
#include "Base/Imgui/ImguiX.h"
#include "Base/Time/Timers.h"
#include "Base/Resource/ResourceID.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
    class UpdateContext;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Skeleton;

    //-------------------------------------------------------------------------

    class AnimationClipBrowser
    {
    public:

        AnimationClipBrowser( ToolsContext const* pToolsContext );

        void Draw( UpdateContext const& context, bool isFocused );

        void SetSkeleton( ResourceID const& skeletonID );

        void ClearFilter();

    private:

        void RebuildCache();
        void UpdateFilter();

    private:

        ToolsContext const*             m_pToolsContext = nullptr;
        ResourceID                      m_skeleton;
        ImGuiX::FilterWidget            m_filter;
        TVector<ResourceID>             m_clips;
        TVector<ResourceID>             m_filteredClips;
        Timer<PlatformClock>            m_refreshTimer;
    };
}