#pragma once

#include "TypeID.h"
#include "CoreTypeIDs.h"
#include "System/Log.h"
#include "System/Types/HashMap.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class EnumInfo
    {
    public:

        struct ConstantInfo
        {
            int64_t m_value = 0;

            #if EE_DEVELOPMENT_TOOLS
            String  m_description;
            #endif
        };

    public:

        inline size_t GetNumValues() const
        {
            return m_constants.size();
        }

        inline bool IsValidValue( StringID label ) const
        {
            auto const iter = m_constants.find( label );
            return iter != m_constants.end();
        }

        inline int64_t GetConstantValue( StringID label ) const
        {
            auto const iter = m_constants.find( label );
            if ( iter != m_constants.end() )
            {
                return iter->second.m_value;
            }
            else // Flag error and return first valid value
            {
                EE_LOG_ERROR( "Serialization", "Invalid enum constant value (%s) for enum (%s)", label.c_str(), m_ID.c_str() );
                return m_constants.begin()->second.m_value;
            }
        }

        inline bool TryGetConstantValue( StringID label, int64_t& outValue ) const
        {
            auto const iter = m_constants.find( label );
            if ( iter != m_constants.end() )
            {
                outValue = iter->second.m_value;
                return true;
            }

            return false;
        }

        inline StringID GetConstantLabel( int64_t value ) const
        {
            for ( auto const& pair : m_constants )
            {
                if ( pair.second.m_value == value )
                {
                    return pair.first;
                }
            }

            EE_UNREACHABLE_CODE();
            return StringID();
        }

        inline bool TryGetConstantLabel( int64_t value, StringID& outValue ) const
        {
            for ( auto const& pair : m_constants )
            {
                if ( pair.second.m_value == value )
                {
                    outValue = pair.first;
                    return true;
                }
            }

            return false;
        }

        #if EE_DEVELOPMENT_TOOLS
        inline String const* TryGetConstantDescription( StringID label ) const
        {
            auto const iter = m_constants.find( label );
            if ( iter != m_constants.end() )
            {
                return &iter->second.m_description;
            }

            return nullptr;
        }
        #endif

    public:

        TypeID                                              m_ID;
        CoreTypeID                                          m_underlyingType;
        THashMap<StringID, ConstantInfo>                    m_constants;
    };
}
