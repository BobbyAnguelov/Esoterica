#pragma once

#include "System/Resource/ResourceProvider.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class ResourceSettings;

    //-------------------------------------------------------------------------

    class EE_SYSTEM_API PackagedResourceProvider final : public ResourceProvider
    {

    public:

        PackagedResourceProvider( ResourceSettings const& settings ) : ResourceProvider( settings ) {}
        virtual bool IsReady() const override final;

    private:

        virtual bool Initialize() override;
        virtual void RequestRawResource( ResourceRequest* pRequest ) override;
        virtual void CancelRequest( ResourceRequest* pRequest ) override;
    };
}