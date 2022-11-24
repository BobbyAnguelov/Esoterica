#pragma once
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceServer;
    class CompilationRequest;

    //-------------------------------------------------------------------------

    class ResourceServerUI final
    {

    public:

        ResourceServerUI( ResourceServer& resourceServer );
        void Draw();

    private:

        void DrawRequests();
        void DrawServerControls();
        void DrawConnectionInfo();
        void DrawPackagingControls();

    private:

        ResourceServer&                         m_resourceServer;
        CompilationRequest const*               m_pSelectedCompletedRequest = nullptr;

        char                                    m_resourcePathbuffer[255] = { 0 };
        bool                                    m_forceRecompilation = false;
    };
}
