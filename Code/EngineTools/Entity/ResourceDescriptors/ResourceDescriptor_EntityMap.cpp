#include "ResourceDescriptor_EntityMap.h"
#include "EngineTools/Entity/EntitySerializationTools.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    void EntityMapResourceDescriptor::Clear()
    {
        m_mapDescriptor.Clear();
    }

    bool EntityMapResourceDescriptor::WriteCustomData( TypeSystem::TypeRegistry const& typeRegistry, Log& log, pugi::xml_node& customDataNode ) const
    {
        return EntityModel::WriteEntityCollectionToXML( typeRegistry, log, m_mapDescriptor, customDataNode );
    }

    bool EntityMapResourceDescriptor::ReadCustomData( TypeSystem::TypeRegistry const& typeRegistry, Log& log, pugi::xml_node& customDataNode )
    {
        return EntityModel::ReadEntityCollectionFromXML( typeRegistry, log, customDataNode, m_mapDescriptor );
    }

    void EntityMapResourceDescriptor::GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const
    {
        if ( subResourceName.empty() )
        {
            TVector<ResourceID> referencedResources;
            m_mapDescriptor.GetAllReferencedResources( referencedResources );

            for ( auto const& referencedResourceID : referencedResources )
            {
                VectorEmplaceBackUnique( outDependencies, referencedResourceID );
            }
        }
        else
        {
            // No install dependencies
        }
    }
}
