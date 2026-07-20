#include "TypeReflection_ReflectedProperty.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    static String GenerateFriendlyName( String const& name )
    {
        String friendlyName = name;

        if ( name.length() <= 1 )
        {
            return friendlyName;
        }

        //-------------------------------------------------------------------------

        StringUtils::ReplaceAllOccurrencesInPlace( friendlyName, "m_", "" );

        if ( friendlyName.length() > 1 && friendlyName[0] == 'p' && isupper( friendlyName[1] ) )
        {
            friendlyName = friendlyName.substr( 1, friendlyName.length() - 1 );
        }

        //-------------------------------------------------------------------------

        StringUtils::ReplaceAllOccurrencesInPlace( friendlyName, "_", " " );
        friendlyName[0] = (char) toupper( friendlyName[0] );
        StringUtils::InsertSpacesAccordingToCapitalization( friendlyName );

        return friendlyName;
    }

    //-------------------------------------------------------------------------

    void ReflectedProperty::GenerateMetaData( Log* pLog )
    {
        EE_ASSERT( pLog != nullptr );

        pLog->ClearLog();
        m_metaData.Clear();
        m_metaData.AppendMetaDataFromString( m_rawMetaDataStr.data(), pLog );

        // Generate friendly name
        //-------------------------------------------------------------------------

        m_reflectedFriendlyName = GenerateFriendlyName( m_name );

        // Create entries for name and category
        //-------------------------------------------------------------------------

        {
            if ( !m_metaData.HasFlag( TypeSystem::PropertyMetadata::FriendlyName ) )
            {
                TypeSystem::PropertyMetadata::KV& nameKV = m_metaData.m_keyValues.emplace_back();
                nameKV.m_key = TypeSystem::PropertyMetadata::Flag::FriendlyName;
                nameKV.m_value = m_reflectedFriendlyName;
                m_metaData.m_flags.SetFlag( nameKV.m_key );
            }

            if ( !m_metaData.HasFlag( TypeSystem::PropertyMetadata::Description ) )
            {
                TypeSystem::PropertyMetadata::KV& descKV = m_metaData.m_keyValues.emplace_back();
                descKV.m_key = TypeSystem::PropertyMetadata::Flag::Description;
                descKV.m_value = m_reflectedDescription;
                m_metaData.m_flags.SetFlag( descKV.m_key );
            }
        }

        EE_ASSERT( m_metaData.HasFlag( TypeSystem::PropertyMetadata::FriendlyName ) );
        EE_ASSERT( m_metaData.HasFlag( TypeSystem::PropertyMetadata::Description ) );

        // Escape description str
        //-------------------------------------------------------------------------

        TypeSystem::PropertyMetadata::KV* pDescription = m_metaData.TryGetEntryForFlag( TypeSystem::PropertyMetadata::Flag::Description );
        EE_ASSERT( pDescription != nullptr );
        StringUtils::ReplaceAllOccurrencesInPlace( pDescription->m_value, "\"", "\\\"" );
    }
}