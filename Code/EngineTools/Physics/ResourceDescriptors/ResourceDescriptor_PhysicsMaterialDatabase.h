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
        EE_DATA_FILE( PhysicsMaterialLibrary, "pml", "Physics Material Library", 0 );

    public:

        EE_REFLECT();
        TVector<Material>         m_materials;
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

        virtual void GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const override
        {
            for( auto const& matLibPath : m_materialLibraries )
            {
                if ( matLibPath.IsValid() )
                {
                    outDependencies.emplace_back( matLibPath, false );
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