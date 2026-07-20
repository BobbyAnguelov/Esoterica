#include "ResourceDescriptor_EntityCollection.h"
#include "EngineTools/Entity/EntitySerializationTools.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    void EntityCollectionResourceDescriptor::Clear()
    {
        m_collection.Clear();
    }

    bool EntityCollectionResourceDescriptor::WriteCustomData( TypeSystem::TypeRegistry const& typeRegistry, Log& log, pugi::xml_node& customDataNode ) const
    {
        return EntityModel::WriteEntityCollectionToXML( typeRegistry, log, m_collection, customDataNode );
    }

    bool EntityCollectionResourceDescriptor::ReadCustomData( TypeSystem::TypeRegistry const& typeRegistry, Log& log, pugi::xml_node& customDataNode )
    {
        return EntityModel::ReadEntityCollectionFromXML( typeRegistry, log, customDataNode, m_collection );
    }

    void EntityCollectionResourceDescriptor::GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const
    {
        TVector<ResourceID> referencedResources;
        m_collection.GetAllReferencedResources( referencedResources );

        for ( auto const& referencedResourceID : referencedResources )
        {
            VectorEmplaceBackUnique( outDependencies, referencedResourceID );
        }
    }
}
