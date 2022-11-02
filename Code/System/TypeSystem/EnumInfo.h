#pragma once

#include "TypeID.h"
#include "CoreTypeIDs.h"
#include "System/Log.h"
#include "System/Types/HashMap.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class EE_SYSTEM_API EnumInfo
    {
    public:

        struct ConstantInfo
        {
            StringID    m_ID;
            int32_t     m_alphabeticalOrder = 0;
            int64_t     m_value = 0;

            #if EE_DEVELOPMENT_TOOLS
            String      m_description;
            #endif
        };

    public:

        inline size_t GetNumConstants() const
        {
            return m_constants.size();
        }

        inline bool IsValidValue( StringID label ) const
        {
            for ( auto const& constant : m_constants )
            {
                if ( constant.m_ID == label )
                {
                    return true;
                }
            }

            return false;
        }

        inline int64_t GetConstantValue( StringID label ) const
        {
            for ( auto const& constant : m_constants )
            {
                if ( constant.m_ID == label )
                {
                    return constant.m_value;
                }
            }

            EE_LOG_ERROR( "Type System", "Serialization", "Invalid enum constant value (%s) for enum (%s)", label.c_str(), m_ID.c_str() );
            return m_constants.begin()->m_value;
        }

        inline bool TryGetConstantValue( StringID label, int64_t& outValue ) const
        {
            for( auto const& constant : m_constants )
            {
                if ( constant.m_ID == label )
                {
                    outValue = constant.m_value;
                    return true;
                }
            }

            return false;
        }

        inline StringID GetConstantLabel( int64_t value ) const
        {
            for ( auto const& constant : m_constants )
            {
                if ( constant.m_value == value )
                {
                    return constant.m_ID;
                }
            }

            EE_UNREACHABLE_CODE();
            return StringID();
        }

        inline bool TryGetConstantLabel( int64_t value, StringID& outValue ) const
        {
            for ( auto const& constant : m_constants )
            {
                if ( constant.m_value == value )
                {
                    outValue = constant.m_ID;
                    return true;
                }
            }

            return false;
        }

        #if EE_DEVELOPMENT_TOOLS
        inline String const* TryGetConstantDescription( StringID label ) const
        {
            for ( auto const& constant : m_constants )
            {
                if ( constant.m_ID == label )
                {
                    return &constant.m_description;
                }
            }

            return nullptr;
        }

        TVector<ConstantInfo> GetConstantsInAlphabeticalOrder() const;
        #endif

    public:

        TypeID                                              m_ID;
        CoreTypeID                                          m_underlyingType;
        TVector<ConstantInfo>                               m_constants;
    };
}
