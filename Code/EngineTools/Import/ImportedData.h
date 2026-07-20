#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Math/Matrix.h"
#include "Base/Types/StringID.h"
#include "Base/Logging/Log.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class EE_ENGINETOOLS_API ImportedData : public Log
    {

    public:

        ImportedData() = default;
        ImportedData( ImportedData const& ) = default;
        virtual ~ImportedData() = default;

        ImportedData& operator=( ImportedData const& rhs ) = default;

        virtual bool IsValid() const = 0;

        inline FileSystem::Path const& GetSourcePath() const { return m_sourcePath; }

    protected:

        FileSystem::Path                    m_sourcePath;
    };
}