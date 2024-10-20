#pragma once

#include "Base/_Module/API.h"
#include "TypeID.h"
#include "Base/Types/Arrays.h"

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
            EE_SERIALIZE( m_propertyID, m_arrayElementIdx );

            PathElement() : m_arrayElementIdx( InvalidIndex ) {}
            PathElement( StringID ID ) : m_propertyID( ID ), m_arrayElementIdx( InvalidIndex ) {}
            PathElement( StringID ID, int32_t arrayElementIdx ) : m_propertyID( ID ), m_arrayElementIdx( arrayElementIdx ) {}

            inline bool IsArrayElement() const { return m_arrayElementIdx != InvalidIndex; }

            inline bool operator==( PathElement const& other ) const
            {
                return m_propertyID == other.m_propertyID && m_arrayElementIdx == other.m_arrayElementIdx;
            }

            inline bool operator!=( PathElement const& other ) const
            {
                return !( *this == other );
            }

        public:

            StringID            m_propertyID;
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
            m_pathElements.emplace_back( PathElement( newElement, arrayElementIdx ) );
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

        inline PropertyPath& operator+=( StringID newElement )
        {
            m_pathElements.emplace_back( PathElement( newElement ) );
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