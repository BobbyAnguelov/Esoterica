#pragma once

#include "Base/Resource/ResourceProvider.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceGlobalSettings;

    //-------------------------------------------------------------------------

    class EE_BASE_API PackagedResourceProvider final : public ResourceProvider
    {

    public:

        PackagedResourceProvider( ResourceGlobalSettings const& settings ) : ResourceProvider( settings ) {}
        virtual bool IsReady() const override final;

    private:

        virtual bool Initialize() override;
        virtual void RequestRawResource( ResourceRequest* pRequest ) override;
        virtual void CancelRequest( ResourceRequest* pRequest ) override;
    };
}