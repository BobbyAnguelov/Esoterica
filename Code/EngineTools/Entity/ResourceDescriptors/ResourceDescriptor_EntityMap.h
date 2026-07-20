#pragma  once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Entity/EntityToolTypes.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Base/Math/Transform.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct EE_ENGINETOOLS_API EntityMapResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( EntityMapResourceDescriptor );

        friend class EntityMapCompiler;

    public:

        virtual bool IsValid() const override { return true; }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual bool RequiresInitialSetup() const override { return false; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return EntityMapDescriptor::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return EntityMapDescriptor::GetStaticResourceTypeID().ToString(); }
        virtual char const* GetFriendlyName() const override final { return EntityMapDescriptor::s_friendlyName; }
        virtual void GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const override;
        virtual void Clear() override;

    private:

        virtual bool SupportsCustomData() const override { return true; }
        virtual bool WriteCustomData( TypeSystem::TypeRegistry const& typeRegistry, Log& log, pugi::xml_node& customDataNode ) const override;
        virtual bool ReadCustomData( TypeSystem::TypeRegistry const& typeRegistry, Log& log, pugi::xml_node& customDataNode ) override;

    public:

        EntityMapDescriptor                 m_mapDescriptor; // Serialized into the custom data of the descriptor

        EE_REFLECT();
        TVector<EntityGroup>                m_entityGroups;

        EE_REFLECT();
        Transform                           m_editorCameraTransform = Transform::Identity;
    };
}