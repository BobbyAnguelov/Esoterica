#include "Animation_TaskSerializer.h"
#include "Animation_PoseTask.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "Base/Encoding/Quantization.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    TaskSerializationContext::TaskSerializationContext( TVector<TypeSystem::TypeInfo const*> const& taskTypes, GraphDefinition const* pGraphDef, TInlineVector<GraphDefinition const*, 3> const &externalGraphDefs, TInlineVector<Skeleton const*, 3> const &secondarySkeletons )
    {
        EE_ASSERT( pGraphDef != nullptr );
        EE_ASSERT( !taskTypes.empty() );

        //-------------------------------------------------------------------------

        m_taskTypes = taskTypes;
        m_maxBitsForTaskTypes = Math::GetMaxNumberOfBitsForValue( m_taskTypes.size() );
        EE_ASSERT( m_maxBitsForTaskTypes <= 8 );

        m_pGraphDef = pGraphDef;
        m_pSkeleton = m_pGraphDef->GetPrimarySkeleton();
        m_secondarySkeletons = secondarySkeletons;
        m_externalGraphDefs = externalGraphDefs;

        //-------------------------------------------------------------------------

        GenerateBoneNameList();
        GenerateBoneMaskList();
        GenerateResourceList();
    }

    TaskSerializationContext::TaskSerializationContext( TVector<TypeSystem::TypeInfo const*> const& taskTypes, Skeleton const* pSkeleton, TInlineVector<Skeleton const*, 3> const& secondarySkeletons, TVector<StringID> const& uniqueBoneNames, TVector<StringID> const& uniqueBoneMaskNames, TVector<Resource::IResource const*> const& uniqueResources )
    {
        EE_ASSERT( m_pSkeleton != nullptr );

        //-------------------------------------------------------------------------

        m_taskTypes = taskTypes;
        m_maxBitsForTaskTypes = Math::GetMaxNumberOfBitsForValue( m_taskTypes.size() );
        EE_ASSERT( m_maxBitsForTaskTypes <= 8 );

        m_pSkeleton = pSkeleton;
        m_secondarySkeletons = secondarySkeletons;

        //-------------------------------------------------------------------------

        PopulateBoneNameList( uniqueBoneNames );
        PopulateBoneMaskList( uniqueBoneMaskNames );
        PopulateResourceList( uniqueResources );
    }

    //-------------------------------------------------------------------------

    void TaskSerializationContext::Clear()
    {
        m_pSkeleton = nullptr;
        m_secondarySkeletons.clear();

        m_taskTypes.clear();
        m_maxBitsForTaskTypes = 0;

        m_boneNameToIdxMap.clear();
        m_uniqueBoneNames.clear();
        m_maxBitsForBoneNames = 0;

        m_boneMaskNameToIdxMap.clear();
        m_uniqueBoneMaskNames.clear();
        m_maxBitsForBoneMaskNames = 0;

        m_resourceIDToIdxMap.clear();
        m_uniqueResources.clear();
        m_maxBitsForResources = 0;
    }

    bool TaskSerializationContext::IsValid() const
    {
        return m_pSkeleton != nullptr && !m_taskTypes.empty();
    }

    uint64_t TaskSerializationContext::CalculateHash() const
    {
        EE_ASSERT( IsValid() );

        TInlineVector<uint64_t, 5000> data;

        for ( TypeSystem::TypeInfo const* pTypeInfo : m_taskTypes )
        {
            data.emplace_back( pTypeInfo->m_ID.ToUint() );
        }

        for ( StringID const &ID : m_uniqueBoneNames )
        {
            data.emplace_back( ID.ToUint() );
        }

        for ( StringID const &ID : m_uniqueBoneMaskNames )
        {
            data.emplace_back( ID.ToUint() );
        }

        for ( Resource::IResource const* pResource : m_uniqueResources )
        {
            data.emplace_back( pResource->GetDataPath().GetID() );
        }

        return Hash::GetHash64( data.data(), data.size() * sizeof( uint64_t ) );
    }

    //-------------------------------------------------------------------------

    uint8_t TaskSerializationContext::GetNumTaskTypes() const
    {
        EE_ASSERT( m_taskTypes.size() < 128 );
        return (uint8_t) m_taskTypes.size();
    }

    uint8_t TaskSerializationContext::GetTaskTypeIndex( PoseTask const* pTask ) const
    {
        EE_ASSERT( pTask != nullptr );

        uint8_t const numTaskTypes = (uint8_t) m_taskTypes.size();
        for ( uint8_t i = 0; i < numTaskTypes; i++ )
        {
            if ( m_taskTypes[i] == pTask->GetTypeInfo() )
            {
                return i;
            }
        }

        return uint8_t( 0xFF );
    }

    PoseTask *TaskSerializationContext::CreateTaskFromTaskTypeIndex( uint8_t nTaskIdx ) const
    {
        if ( nTaskIdx >= m_taskTypes.size() )
        {
            return nullptr;
        }

        return (PoseTask*) m_taskTypes[nTaskIdx]->CreateType();
    }

    //-------------------------------------------------------------------------

    int32_t TaskSerializationContext::GetBoneNameIndex( StringID boneNameID ) const
    {
        auto iter = m_boneNameToIdxMap.find( boneNameID );
        if ( iter != m_boneNameToIdxMap.end() )
        {
            return iter->second;
        }

        return InvalidIndex;
    }

    void TaskSerializationContext::GenerateBoneNameList()
    {
        auto AddSkeletonBones = [this] ( Skeleton const *pSkeleton )
        {
            EE_ASSERT( pSkeleton != nullptr );

            int32_t const numBones = pSkeleton->GetNumBones();
            for ( int32_t boneIdx = 0; boneIdx < numBones; boneIdx++ )
            {
                // We only create records for unique resources!
                StringID const boneID = pSkeleton->GetBoneID( boneIdx );
                if ( m_boneNameToIdxMap.find( pSkeleton->GetBoneID( boneIdx ) ) == m_boneNameToIdxMap.end() )
                {
                    int32_t const boneNameIdx = (int32_t) m_uniqueBoneNames.size();
                    m_uniqueBoneNames.emplace_back( boneID );
                    m_boneNameToIdxMap.insert( TPair<StringID, int32_t>( boneID, boneNameIdx ) );
                }
            }
        };

        EE_ASSERT( m_pSkeleton != nullptr );
        AddSkeletonBones( m_pSkeleton );

        // Sort the secondary skeletons so that the set of bones is always deterministic
        eastl::sort( m_secondarySkeletons.begin(), m_secondarySkeletons.end(), [] ( Skeleton const* pA, Skeleton const* pB ) { return StringUtils::Stricmp( pA->GetResourceID().GetDataPath().c_str(), pB->GetResourceID().GetDataPath().c_str() ) < 0; } );

        for ( Skeleton const* pSecondarySkeleton : m_secondarySkeletons )
        {
            EE_ASSERT( pSecondarySkeleton != nullptr );
            AddSkeletonBones( pSecondarySkeleton );
        }

        m_maxBitsForBoneNames = Math::GetMaxNumberOfBitsForValue( m_uniqueBoneNames.size() );
        EE_ASSERT( m_maxBitsForBoneNames <= 16 );
    }

    void TaskSerializationContext::PopulateBoneNameList( TVector<StringID> const &uniqueBoneNames )
    {
        for ( StringID const& boneID : uniqueBoneNames )
        {
            int32_t const boneNameIdx = (int32_t) m_uniqueBoneNames.size();
            m_uniqueBoneNames.emplace_back( boneID );
            m_boneNameToIdxMap.insert( TPair<StringID, int32_t>( boneID, boneNameIdx ) );
        }

        m_maxBitsForBoneNames = Math::GetMaxNumberOfBitsForValue( m_uniqueBoneNames.size() );
        EE_ASSERT( m_maxBitsForBoneNames <= 16 );
    }

    int32_t TaskSerializationContext::GetBoneMaskIndex( StringID boneMaskID ) const
    {
        auto iter = m_boneMaskNameToIdxMap.find( boneMaskID );
        if ( iter != m_boneMaskNameToIdxMap.end() )
        {
            return iter->second;
        }

        return InvalidIndex;
    }

    void TaskSerializationContext::GenerateBoneMaskList()
    {
        EE_ASSERT( m_pSkeleton != nullptr );

        int32_t const numBoneMasks = (int32_t) m_pSkeleton->GetNumBoneMaskSets();
        for ( int32_t i = 0; i < numBoneMasks; i++ )
        {
            // We only create records for unique resources!
            StringID const boneMaskID = m_pSkeleton->GetBoneMaskSetID( i );
            if ( m_boneMaskNameToIdxMap.find( boneMaskID ) == m_boneMaskNameToIdxMap.end() )
            {
                int32_t const boneMaskIdx = (int32_t) m_uniqueBoneMaskNames.emplace_back( boneMaskID );
                m_boneMaskNameToIdxMap.insert( TPair<StringID, int32_t>( boneMaskID, boneMaskIdx ) );
            }
        }

        m_maxBitsForBoneMaskNames = Math::GetMaxNumberOfBitsForValue( m_uniqueBoneMaskNames.size() );
        EE_ASSERT( m_maxBitsForBoneMaskNames <= 8 );
    }

    void TaskSerializationContext::PopulateBoneMaskList( TVector<StringID> const &uniqueBoneMaskNames )
    {
        for ( StringID boneMaskID : uniqueBoneMaskNames )
        {
            int32_t const boneMaskIdx = (int32_t) m_uniqueBoneMaskNames.emplace_back( boneMaskID );
            m_boneMaskNameToIdxMap.insert( TPair<StringID, int32_t>( boneMaskID, boneMaskIdx ) );
        }

        m_maxBitsForBoneMaskNames = Math::GetMaxNumberOfBitsForValue( m_uniqueBoneMaskNames.size() );
        EE_ASSERT( m_maxBitsForBoneMaskNames <= 8 );
    }

    //-------------------------------------------------------------------------

    int32_t TaskSerializationContext::GetResourceIndex( ResourceID const& resourceID ) const
    {
        auto iter = m_resourceIDToIdxMap.find( resourceID );
        if ( iter != m_resourceIDToIdxMap.end() )
        {
            return iter->second;
        }

        return InvalidIndex;
    }

    void TaskSerializationContext::GenerateResourceList()
    {
        m_resourceIDToIdxMap.clear();
        m_uniqueResources.clear();

        //-------------------------------------------------------------------------

        TInlineVector<ResourceLUT const*, 10> LUTs;

        EE_ASSERT( m_pSkeleton != nullptr );
        LUTs.emplace_back( &m_pGraphDef->GetResourceLookupTable() );

        int32_t estimatedCapacity = int32_t( m_pGraphDef->GetResourceLookupTable().size() );
        for ( auto const &pExternalGraphDef : m_externalGraphDefs )
        {
            EE_ASSERT( pExternalGraphDef != nullptr );
            ResourceLUT const &externalGraphLut = pExternalGraphDef->GetResourceLookupTable();
            LUTs.emplace_back( &externalGraphLut );
            estimatedCapacity += (int32_t) externalGraphLut.size();
        }

        m_resourceIDToIdxMap.reserve( estimatedCapacity );
        m_uniqueResources.reserve( estimatedCapacity );

        //-------------------------------------------------------------------------

        for ( ResourceLUT const *pLUT : LUTs )
        {
            for ( auto const& pair : *pLUT )
            {
                TryAddResourceToUniqueResources( pair.second.GetPtr<Resource::IResource>() );
            }
        }

        //-------------------------------------------------------------------------

        m_maxBitsForResources = Math::GetMaxNumberOfBitsForValue( m_uniqueResources.size() );
        EE_ASSERT( m_maxBitsForResources <= 16 );
    }

    void TaskSerializationContext::PopulateResourceList( TVector<Resource::IResource const*> const &uniqueResources )
    {
        for ( auto const &pResource : uniqueResources )
        {
            TryAddResourceToUniqueResources( pResource );
        }

        m_maxBitsForResources = Math::GetMaxNumberOfBitsForValue( m_uniqueResources.size() );
        EE_ASSERT( m_maxBitsForResources <= 16 );
    }

    //-------------------------------------------------------------------------

    TaskSerializer::TaskSerializer( TaskSerializationContext const& taskSerializationContext )
        : m_context( taskSerializationContext )
    {}

    TaskSerializer::TaskSerializer( TaskSerializationContext const& taskSerializationContext, Blob const& inTopologyData, Blob const& inTaskData )
        : Serialization::BitArchive( inTaskData )
        , m_context( taskSerializationContext )
        , m_topologyData( inTopologyData )
    {}

    //-------------------------------------------------------------------------

    void TaskSerializer::WriteRotation( Quaternion const& rotation )
    {
        EE_ASSERT( rotation.IsNormalized() );

        Quantization::EncodedQuaternion q( rotation );
        WriteUInt( q.m_data0, 16 );
        WriteUInt( q.m_data1, 16 );
        WriteUInt( q.m_data2, 16 );
    }

    Quaternion TaskSerializer::ReadRotation()
    {
        Quantization::EncodedQuaternion q;
        q.m_data0 = (uint16_t) ReadUInt( 16 );
        q.m_data1 = (uint16_t) ReadUInt( 16 );
        q.m_data2 = (uint16_t) ReadUInt( 16 );
        return q.ToQuaternion();
    }

    int32_t TaskSerializer::ReadBoneIndex( bool bRestrictToPrimarySkeleton, Skeleton const **ppOptionalOutSkeletonForIndex )
    {
        StringID const targetBoneNameID = ReadBoneName();

        int32_t boneIdx = InvalidIndex;

        // Check primary skeleton
        //-------------------------------------------------------------------------

        if ( ppOptionalOutSkeletonForIndex != nullptr )
        {
            *ppOptionalOutSkeletonForIndex = m_context.GetSkeleton();
        }

        boneIdx = m_context.GetSkeleton()->GetBoneIndex( targetBoneNameID );

        // Check secondary skeletons if we are allowed to and we didnt find the bone in the primary skeleton
        //-------------------------------------------------------------------------

        if ( !bRestrictToPrimarySkeleton && boneIdx == InvalidIndex )
        {
            for ( auto pSecondarySkeleton : m_context.GetSecondarySkeletons() )
            {
                boneIdx = pSecondarySkeleton->GetBoneIndex( targetBoneNameID );
                if ( boneIdx != InvalidIndex )
                {
                    if ( ppOptionalOutSkeletonForIndex != nullptr )
                    {
                        *ppOptionalOutSkeletonForIndex = pSecondarySkeleton;
                    }

                    break;
                }
            }
        }

        return boneIdx;
    }
}