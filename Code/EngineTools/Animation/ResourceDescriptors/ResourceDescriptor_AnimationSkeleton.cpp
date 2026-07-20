#include "ResourceDescriptor_AnimationSkeleton.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    bool SkeletonResourceDescriptor::ValidateData( Log* pLog ) const
    {
        for ( auto i = 0; i < m_boneMaskSetDefinitions.size(); i++ )
        {
            for ( auto j = i + 1; j < m_boneMaskSetDefinitions.size(); j++ )
            {
                if ( m_boneMaskSetDefinitions[i].m_ID == m_boneMaskSetDefinitions[j].m_ID )
                {
                    if ( pLog != nullptr )
                    {
                        pLog->LogError( "Duplicate bone mask set ID detected: %s", m_boneMaskSetDefinitions[i].m_ID.c_str() );
                    }

                    return false;
                }
            }
        }

        //-------------------------------------------------------------------------

        for ( auto i = 0; i < m_floatChannelSets.size(); i++ )
        {
            for ( auto j = i + 1; j < m_floatChannelSets.size(); j++ )
            {
                if ( m_floatChannelSets[i].m_ID == m_floatChannelSets[j].m_ID )
                {
                    if ( pLog != nullptr )
                    {
                        pLog->LogError( "Duplicate float channel set ID detected: %s", m_floatChannelSets[i].m_ID.c_str() );
                    }

                    return false;
                }
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }

    StringID SkeletonResourceDescriptor::GetUniqueBoneMaskSetID( String const& desiredName ) const
    {
        EE_ASSERT( !desiredName.empty() );

        StringID desiredID( desiredName );

        bool isUniqueName = false;
        while ( !isUniqueName )
        {
            isUniqueName = true;

            for ( auto const& bms : m_boneMaskSetDefinitions )
            {
                if ( desiredID == bms.m_ID )
                {
                    isUniqueName = false;
                    break;
                }
            }

            //-------------------------------------------------------------------------

            if ( !isUniqueName )
            {
                InlineString str = desiredName.c_str();
                str.append( "_" );
                desiredID = StringID( str.c_str() );
            }
        }

        return desiredID;
    }

    StringID SkeletonResourceDescriptor::GetUniqueFloatChannelSetID( String const& desiredName ) const
    {
        EE_ASSERT( !desiredName.empty() );

        StringID desiredID( desiredName );

        bool isUniqueName = false;
        while ( !isUniqueName )
        {
            isUniqueName = true;

            for ( auto const& floatChannelSet : m_floatChannelSets)
            {
                if ( desiredID == floatChannelSet.m_ID )
                {
                    isUniqueName = false;
                    break;
                }
            }

            //-------------------------------------------------------------------------

            if ( !isUniqueName )
            {
                InlineString str = desiredName.c_str();
                str.append( "_" );
                desiredID = StringID( str.c_str() );
            }
        }

        return desiredID;
    }

    void SkeletonResourceDescriptor::PostLoad( TypeSystem::TypeRegistry const& typeRegistry )
    {
        Resource::ResourceDescriptor::PostLoad( typeRegistry );

        // Fix any duplicate mask set IDs and remove duplicate bone weights set IDs
        //-------------------------------------------------------------------------

        for ( auto& bms : m_boneMaskSetDefinitions )
        {
            if ( !bms.m_ID.IsValid() )
            {
                bms.m_ID = GetUniqueBoneMaskSetID( "Bone Mask Set" );
            }
        }

        for ( int32_t i = 0; i < m_boneMaskSetDefinitions.size(); i++ )
        {
            for ( int32_t j = i + 1; j < m_boneMaskSetDefinitions.size(); j++ )
            {
                if ( m_boneMaskSetDefinitions[i].m_ID == m_boneMaskSetDefinitions[j].m_ID )
                {
                    m_boneMaskSetDefinitions[j] = GetUniqueBoneMaskSetID( m_boneMaskSetDefinitions[j].m_ID.c_str() );
                }
            }
        }

        for ( auto& boneMaskSetDefinition : m_boneMaskSetDefinitions )
        {
            boneMaskSetDefinition.m_primaryWeightList.RemoveDuplicateWeights();

            for ( auto& secondaryWeightList : boneMaskSetDefinition.m_secondaryWeightLists )
            {
                secondaryWeightList.RemoveDuplicateWeights();
            }
        }

        // Fix any duplicate set IDs and remove duplicate channel set IDs
        //-------------------------------------------------------------------------

        for ( auto& fcs : m_floatChannelSets )
        {
            if ( !fcs.m_ID.IsValid() )
            {
                fcs.m_ID = GetUniqueFloatChannelSetID( "Float Channel Set" );
            }
        }

        for ( int32_t i = 0; i < m_floatChannelSets.size(); i++ )
        {
            for ( int32_t j = i + 1; j < m_floatChannelSets.size(); j++ )
            {
                if ( m_floatChannelSets[i].m_ID == m_floatChannelSets[j].m_ID )
                {
                    m_floatChannelSets[j] = GetUniqueBoneMaskSetID( m_floatChannelSets[j].m_ID.c_str() );
                }
            }
        }

        for ( auto& floatChannelSet : m_floatChannelSets )
        {
            floatChannelSet.RemoveDuplicateChannelIDs();
        }
    }
}