#pragma once

#include "Base/_Module/API.h"
#include "TypeID.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------
// The path to a property within a reflected type
//-------------------------------------------------------------------------
// Examples:
//  "Foo/Bar" -> set the bar value of nested type foo
//  "Foo/BarArray/0" -> set the first value of BarArray of nested type foo

namespace EE::TypeSystem
{
    class EE_BASE_API PropertyPath
    {
        EE_CUSTOM_SERIALIZE_WRITE_FUNCTION( archive )
        {
            archive << m_pathElements;
            return archive;
        }

        EE_CUSTOM_SERIALIZE_READ_FUNCTION( archive )
        {
            archive << m_pathElements;
            GenerateHashCode();
            return archive;
        }

    public:

        struct PathElement
        {
            EE_SERIALIZE( m_ID, m_arrayElementIdx );

            PathElement() : m_arrayElementIdx( InvalidIndex ) {}
            PathElement( StringID ID ) : m_ID( ID ), m_arrayElementIdx( InvalidIndex ) {}
            PathElement( StringID ID, int32_t arrayElementIdx ) : m_ID( ID ), m_arrayElementIdx( arrayElementIdx ) {}

            inline bool IsArrayElement() const { return m_arrayElementIdx != InvalidIndex; }

            inline bool operator==( PathElement const& other ) const
            {
                return m_ID == other.m_ID && m_arrayElementIdx == other.m_arrayElementIdx;
            }

            inline bool operator!=( PathElement const& other ) const
            {
                return !( *this == other );
            }

            inline bool operator<( PathElement const& rhs ) const
            {
                EE_ASSERT( m_ID.IsValid() && rhs.m_ID.IsValid() );

                int32_t const result = StringUtils::Stricmp( m_ID.c_str(), rhs.m_ID.c_str() );
                if ( result < 0 )
                {
                    return true;
                }

                if ( result == 0 )
                {
                    return m_arrayElementIdx < rhs.m_arrayElementIdx;
                }

                return false;
            }

        public:

            StringID            m_ID;
            int32_t             m_arrayElementIdx;
        };

    public:

        PropertyPath() {}
        PropertyPath( String const& pathString );

        inline bool IsValid() const
        {
            return !m_pathElements.empty();
        }

        inline uint64_t GetHashCode() const
        {
            return m_hash;
        }

        void Clear()
        {
            m_pathElements.clear();
            m_hash = 0;
        }

        inline size_t GetNumElements() const
        {
            return m_pathElements.size();
        }

        inline PathElement const& FirstElement() const
        {
            EE_ASSERT( IsValid() );
            return m_pathElements.front();
        }

        inline PathElement const& GetLastElement() const
        {
            EE_ASSERT( IsValid() );
            return m_pathElements.back();
        }

        inline bool IsPathToArrayElement() const
        {
            EE_ASSERT( IsValid() );
            return GetLastElement().IsArrayElement();
        }

        inline void Append( StringID newElement, int32_t arrayElementIdx = InvalidIndex )
        {
            EE_ASSERT( newElement.IsValid() && arrayElementIdx >= InvalidIndex );
            m_pathElements.emplace_back( newElement, arrayElementIdx );
            GenerateHashCode();
        }

        inline void RemoveLastElement()
        {
            EE_ASSERT( IsValid() );
            m_pathElements.pop_back();
            GenerateHashCode();
        }

        inline void ReplaceLastElement( StringID newElement, int32_t arrayElementIdx = InvalidIndex )
        {
            EE_ASSERT( IsValid() );
            EE_ASSERT( newElement.IsValid() && arrayElementIdx >= InvalidIndex );
            m_pathElements.back() = PathElement( newElement, arrayElementIdx );
            GenerateHashCode();
        }

        inline PropertyPath GetPathWithoutFirstElement() const
        {
            EE_ASSERT( IsValid() );
            EE_ASSERT( m_pathElements.size() > 1 );
            PropertyPath subPath;
            subPath.m_pathElements = TInlineVector<PathElement, 10>( m_pathElements.begin() + 1, m_pathElements.end() );
            subPath.GenerateHashCode();
            return subPath;
        }

        inline PathElement const& operator[]( size_t idx ) const
        {
            EE_ASSERT( idx < m_pathElements.size() );
            return m_pathElements[idx];
        }

        inline bool operator==( PropertyPath const& other ) const
        {
            return m_hash == other.m_hash;
        }

        inline bool operator!=( PropertyPath const& other ) const
        {
            return m_hash != other.m_hash;
        }

        inline bool operator<( PropertyPath const& rhs ) const
        {
            if ( m_pathElements.size() != rhs.m_pathElements.size() )
            {
                return ( m_pathElements.size() < rhs.m_pathElements.size() );
            }

            size_t numElements = m_pathElements.size();
            for ( size_t i = 0; i < numElements; i++ )
            {
                if ( m_pathElements[i] < rhs.m_pathElements[i] )
                {
                    return true;
                }
                else if ( m_pathElements[i] != rhs.m_pathElements[i] )
                {
                    return false;
                }
            }

            // If we get here, it means all elements are equal
            return false;
        }

        inline PropertyPath& operator+=( StringID newElement )
        {
            m_pathElements.emplace_back( newElement );
            GenerateHashCode();
            return *this;
        }

        String ToString() const;

    private:

        void GenerateHashCode();

    private:

        TInlineVector<PathElement, 10>      m_pathElements;
        uint64_t                            m_hash = 0;
    };
}

//-------------------------------------------------------------------------

namespace eastl
{
    template <typename T> struct hash;

    template <>
    struct hash<EE::TypeSystem::PropertyPath>
    {
        size_t operator()( EE::TypeSystem::PropertyPath const& path ) const
        {
            return path.GetHashCode();
        }
    };
}