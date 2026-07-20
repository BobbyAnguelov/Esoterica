#pragma once
#if EE_ENABLE_NAVPOWER

#include <bfxBuilder.h>
#include "Base/Logging/Log.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    class NavmeshBuildData;
    class NavmeshData;

    //-------------------------------------------------------------------------

    class NavmeshBuilder : public Log, public bfx::BuildProgressMonitor
    {
    public:

        bool Build( NavmeshBuildData const& buildData, NavmeshData& outData );

        inline float GetProgress() const { return m_progress; }

    private:

        virtual void BuildProgressUpdate( float percentDone ) override { m_progress = percentDone / 100.0f; }

        bool CollectTriangles( NavmeshBuildData const& buildData );

        bool BuildNavmesh( NavmeshBuildData const& buildData, NavmeshData& navmeshData );

    private:

        bfx::Instance*                                  m_pNavpowerInstance = nullptr;
        TVector<bfx::BuildFace>                         m_buildFaces;
        float                                           m_progress = 0.0f;
    };
}

#endif