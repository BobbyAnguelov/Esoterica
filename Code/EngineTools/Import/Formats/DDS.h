#pragma once

#include "FormatFile.h"

//-------------------------------------------------------------------------

namespace EE::Import::DDS
{
    class EE_ENGINETOOLS_API File final : public Import::ImageFile
    {
    public:

        File( Source const& source ) : Import::ImageFile( source ) {}
        virtual TUniquePtr<Image> ReadImage() const override;
    };
}
