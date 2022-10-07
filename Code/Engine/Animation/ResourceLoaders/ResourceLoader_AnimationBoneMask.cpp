#include "ResourceLoader_AnimationBoneMask.h"
#include "Engine/Animation/AnimationBoneMask.h"
#include "System/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    BoneMaskLoader::BoneMaskLoader()
    {
        m_loadableTypes.push_back( BoneMaskDefinition::GetStaticResourceTypeID() );
    }

    bool BoneMaskLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        BoneMaskDefinition* pBoneMaskDefinition = EE::New<BoneMaskDefinition>();
        archive << *pBoneMaskDefinition;
        EE_ASSERT( pBoneMaskDefinition->IsValid() );
        pResourceRecord->SetResourceData( pBoneMaskDefinition );
        return true;
    }
}