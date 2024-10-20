#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Physics/PhysicsMaterial.h"
#include "Base/FileSystem/IDataFile.h"

//-------------------------------------------------------------------------

namespace EE::Physics
{
    struct EE_ENGINETOOLS_API PhysicsMaterialLibrary : public IDataFile
    {
        EE_DATA_FILE( PhysicsMaterialLibrary, 'pml', "Physic Material Library", 0 );

    public:

        EE_REFLECT();
        TVector<MaterialSettings>         m_settings;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API PhysicsMaterialDatabaseResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( PhysicsMaterialDatabaseResourceDescriptor );

        virtual bool IsValid() const override { return true; }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return MaterialDatabase::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return MaterialDatabase::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return MaterialDatabase::s_friendlyName; }

        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override
        {
            for( auto const& matLibPath : m_materialLibraries )
            {
                if ( matLibPath.IsValid() )
                {
                    outDependencies.emplace_back( matLibPath );
                }
            }
        }

        virtual void Clear() override
        {
            m_materialLibraries.clear();
        }

    public:

        EE_REFLECT();
        TVector<DataPath>         m_materialLibraries;
    };
}