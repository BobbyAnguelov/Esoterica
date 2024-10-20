#pragma once
#include "Base/Types/StringID.h"
#include "Base/Types/String.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Arrays.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------
// Categorized IDs
//-------------------------------------------------------------------------
// Categorized 4-Level( 128bit ) tag consisting of 4 StringIDs
// Categories must be sequentially set for the tag to be valid, you cannot have an invalid tag between valid IDs.

namespace EE
{
    template<size_t N>
    class TTag
    {
        EE_SERIALIZE( m_c );
        static_assert( N > 1, "Tags only makes sense for more than 1 level" );
        static_assert( N <= 4, "We dont allow tags larger than 4 levels" );

    public:

        using QueryResults = TInlineVector<TTag<N>, 5>;

        // Utility Functions
        //-------------------------------------------------------------------------

        // Create a tag from a string in the format "C0.C1.C2.C3..."
        static TTag FromString( String const& str );

        // Does this container have a tag that exactly matches the specified tag
        template<typename T>
        static bool HasExactlyMatchingID( T const& container, TTag const& IDToMatch )
        {
            for ( TTag const& tag : container )
            {
                if ( tag == IDToMatch )
                {
                    return true;
                }
            }

            return false;
        }

        // Ensure that the supplied container doesnt have any duplicate tag
        template<typename T>
        static void EnsureUniqueID( T& container )
        {
            auto SortPredicate = [] ( TTag const& a, TTag const& b )
            {
                for ( int32_t i = 0; i < N; i++ )
                {
                    if ( a.m_c[i] < b.m_c[i] )
                    {
                        return true;
                    }

                    if ( a.m_c[i] > b.m_c[i] )
                    {
                        return false;
                    }
                }

                return false;
            };

            eastl::sort( container.begin(), container.end(), SortPredicate );
            auto lastIter = eastl::unique( container.begin(), container.end() );
            container.erase( lastIter, container.end() );
        }

        // Get all the child IDs for a given parent in this container, returns true if any children were found.
        template<typename T>
        static bool FindAllChildCategories( T const& container, TTag const& parentID, QueryResults& outResults )
        {
            bool childrenWereFound = false;
            outResults.clear();

            for ( TTag const& tag : container )
            {
                if ( tag.IsChildOf( parentID ) )
                {
                    outResults.emplace_back( tag );
                    childrenWereFound = true;
                }
            }

            return childrenWereFound;
        }

        // Does this container has any IDs that are children of the supplied tag
        template<typename T>
        static bool HasAnyChildCategories( T const& container, TTag const& parentID )
        {
            for ( TTag const& tag : container )
            {
                if ( tag.IsChildOf( parentID ) )
                {
                    return true;
                }
            }

            return false;
        }

    public:

        TTag() { Memory::MemsetZero( m_c, sizeof( StringID ) * N ); }

        // Create a tag from a list of strings
        explicit TTag( std::initializer_list<char const*> args )
        {
            EE_ASSERT( args.size() <= N );
            int32_t idx = 0;
            for ( auto elem : args )
            {
                EE_ASSERT( elem != nullptr );
                m_c[idx] = StringID( elem );
                idx++;
            }

            for ( ; idx < N; idx++ )
            {
                m_c[idx] = StringID();
            }
        }

        // Create a tag from a list of StringIDs
        explicit TTag( std::initializer_list<StringID> args )
        {
            EE_ASSERT( args.size() <= N );
            int32_t idx = 0;
            for ( auto elem : args )
            {
                EE_ASSERT( elem.IsValid() );
                m_c[idx] = elem;
                idx++;
            }

            for ( ; idx < N; idx++ )
            {
                m_c[idx] = StringID();
            }
        }

        TTag( TTag const& tag ) { CopyFromTag( tag ); }

        TTag( TTag&& tag ) { CopyFromTag( tag ); }

        TTag& operator=( TTag const& tag ) { CopyFromTag( tag ); return *this; }

        TTag& operator=( TTag&& tag ) { CopyFromTag( tag ); return *this; }

        //-------------------------------------------------------------------------

        // Is this tag valid? Categories must be sequentially set for the tag to be valid, you cannot have an invalid tag between valid IDs.
        bool IsValid() const;

        // Get the tag as a string in the following format: "C0.C1.C2.C3"
        InlineString ToString() const;

        // Get the current tag depth (i.e. how many levels have been set)
        int32_t GetDepth() const;

        // Accessors
        //-------------------------------------------------------------------------

        // Get the parent tag, i.e. the tag with one less category set
        TTag GetParentTag() const;

        // Get specific ID at the specified depth
        StringID GetValueN( uint32_t depth ) const
        {
            EE_ASSERT( depth >= 0 && depth <= ( N - 1 ) );
            EE_ASSERT( IsValid() );
            return m_c[depth];
        }

        // Get specific ID at the specified depth
        EE_FORCE_INLINE StringID GetValueAtDepth( uint32_t depth ) const { return GetValueN( depth ); }

        // Get specific ID at the specified depth
        EE_FORCE_INLINE StringID operator[]( int32_t depth ) const { return GetValueN( depth ); }

        // Set specific ID at the specified depth
        void SetValueN( uint32_t depth, StringID ID )
        {
            EE_ASSERT( depth >= 0 && depth <= ( N - 1 ) );
            EE_ASSERT( ( ( depth + 1 ) < ( N - 1 ) ) ? !m_c[depth + 1 ].IsValid() : true );
            m_c[depth] = ID;
            EE_ASSERT( IsValid() );
        }

        // Set specific ID at the specified depth
        EE_FORCE_INLINE void SetValueAtDepth( uint32_t depth, StringID ID ) { return SetValueN( depth, ID ); }

        // Queries and Comparisons
        //-------------------------------------------------------------------------

        // Match only the first N level of IDs
        inline bool MatchN( TTag const& rhs, uint32_t n ) const
        {
            EE_ASSERT( n >= 1 && n <= N );

            for ( uint32_t i = 0; i < n; i++ )
            {
                if ( m_c[i] != rhs.m_c[i] )
                {
                    return false;
                }
            }

            return true;
        }

        // Are these Tags EXACTLY equal
        inline bool operator==( TTag const& rhs ) const
        {
            return MatchN( rhs, N );
        }

        // Are these Tags different
        inline bool operator!=( TTag const& rhs ) const
        {
            return !MatchN( rhs, N );
        }

        // Is this tag a parent of the specified tag? e.g., A.B is a parent to A.B.C and A.B.Q
        bool IsParentOf( TTag const& childID ) const;

        // Is this tag a child of the specified tag? e.g., A.B.C derives from A and from A.B
        bool IsChildOf( TTag const& parentID ) const;

        EE_FORCE_INLINE void CopyFromTag( TTag const& tag ) { memcpy( m_c, tag.m_c, sizeof( StringID ) * N ); }

    protected:

        StringID m_c[N];
    };

    // Implementation
    //-------------------------------------------------------------------------

    template<size_t N>
    TTag<N> TTag<N>::FromString( String const& string )
    {
        TTag tag;

        //-------------------------------------------------------------------------

        String tempString;
        size_t idx, lastIdx = 0;
        for ( int32_t i = 0; i < N; i++ )
        {
            idx = string.find_first_of( '.', lastIdx );
            if ( idx == String::npos )
            {
                idx = string.length();

                if ( idx != lastIdx )
                {
                    tempString.assign( string.data() + lastIdx, idx - lastIdx );
                    tag.m_c[i] = StringID( tempString );
                }
                break;
            }
            else
            {
                if ( idx != lastIdx )
                {
                    tempString.assign( string.data() + lastIdx, idx - lastIdx );
                    tag.m_c[i] = StringID( tempString );
                }
            }

            lastIdx = idx + 1;
        }

        return tag;
    }

    template<size_t N>
    bool TTag<N>::IsValid() const
    {
        for ( int32_t i = ( N - 1 ); i >= 0; i-- )
        {
            // If we have a value set
            if ( m_c[i].IsValid() )
            {
                // Ensure all previous values are set
                for ( auto j = i - 1; j >= 0; j-- )
                {
                    if ( !m_c[j].IsValid() )
                    {
                        return false;
                    }
                }

                return true;
            }
        }

        return false;
    }

    template<size_t N>
    int32_t TTag<N>::GetDepth() const
    {
        for ( int32_t i = ( N - 1 ); i >= 0; i-- )
        {
            // If we have a value set
            if ( m_c[i].IsValid() )
            {
                // Ensure all previous values are set
                for ( int32_t j = i - 1; j >= 0; j-- )
                {
                    if ( !m_c[j].IsValid() )
                    {
                        return 0;
                    }
                }

                return i + 1;
            }
        }

        return 0;
    }

    template<size_t N>
    TTag<N> TTag<N>::GetParentTag() const
    {
        TTag parentID;
        int32_t const parentDepth = GetDepth() - 1;
        for ( int32_t idx = 0; idx < N; idx++ )
        {
            if ( idx < parentDepth )
            {
                parentID.m_c[idx] = m_c[idx];
            }
            else
            {
                parentID.m_c[idx] = StringID();
            }
        }

        return parentID;
    }

    template<size_t N>
    bool TTag<N>::IsParentOf( TTag<N> const& childTag ) const
    {
        EE_ASSERT( IsValid() && childTag.IsValid() );

        for ( int32_t i = 0; i < N; i++ )
        {
            if ( !m_c[i].IsValid() && childTag.m_c[i].IsValid() )
            {
                return true;
            }

            if ( m_c[i] != childTag.m_c[i] )
            {
                return false;
            }
        }

        return false;
    }

    template<size_t N>
    bool TTag<N>::IsChildOf( TTag<N> const& parentTag ) const
    {
        EE_ASSERT( IsValid() && parentTag.IsValid() );

        for ( int32_t i = 0; i < N; i++ )
        {
            if ( m_c[i].IsValid() && !parentTag.m_c[i].IsValid() )
            {
                return true;
            }

            if ( m_c[i] != parentTag.m_c[i] )
            {
                return false;
            }
        }

        return false;
    }

    template<size_t N>
    InlineString TTag<N>::ToString() const
    {
        // For now hard-coded to reduce the number of sprintf/appends
        InlineString result;
        uint32_t const depth = GetDepth();
        switch ( depth )
        {
            case 1:
            {
                result.sprintf( "%s", m_c[0].c_str() );
            }
            break;

            case 2:
            {
                result.sprintf( "%s.%s", m_c[0].c_str(), m_c[1].c_str() );
            }
            break;

            case 3:
            {
                result.sprintf( "%s.%s.%s", m_c[0].c_str(), m_c[1].c_str(), m_c[2].c_str() );
            }
            break;

            case 4:
            {
                result.sprintf( "%s.%s.%s.%s", m_c[0].c_str(), m_c[1].c_str(), m_c[2].c_str(), m_c[3].c_str() );
            }
            break;
        }

        return result;
    }

    // Common Use cases
    //-------------------------------------------------------------------------

    class Tag2 : public TTag<2>
    {
    public:

        Tag2( char const* p0, char const* p1 = nullptr )
            : TTag<2>()
        {
            if ( p0 != nullptr ) { m_c[0] = StringID( p0 ); } else { return; }
            if ( p1 != nullptr ) { m_c[1] = StringID( p1 ); } else { return; }
        }

        Tag2( StringID s0, StringID s1 = StringID() )
            : TTag<2>()
        {
            if ( s0.IsValid() ) { m_c[0] = s0; } else { return; }
            if ( s1.IsValid() ) { m_c[1] = s1; } else { return; }
        }

        using TTag::TTag;
        using TTag::operator =;
    };

    class Tag4 : public TTag<4>
    {
    public:

        Tag4( char const* p0, char const* p1 = nullptr, char const* p2 = nullptr, char const* p3 = nullptr )
            : TTag<4>()
        {
            if ( p0 != nullptr ) { m_c[0] = StringID( p0 ); } else { return; }
            if ( p1 != nullptr ) { m_c[1] = StringID( p1 ); } else { return; }
            if ( p2 != nullptr ) { m_c[2] = StringID( p2 ); } else { return; }
            if ( p3 != nullptr ) { m_c[3] = StringID( p3 ); } else { return; }
            EE_ASSERT( IsValid() );
        }

        Tag4( StringID s0, StringID s1 = StringID(), StringID s2 = StringID(), StringID s3 = StringID() )
            : TTag<4>()
        {
            if ( s0.IsValid() ) { m_c[0] = s0; } else { return; }
            if ( s1.IsValid() ) { m_c[1] = s1; } else { return; }
            if ( s2.IsValid() ) { m_c[2] = s2; } else { return; }
            if ( s3.IsValid() ) { m_c[3] = s3; } else { return; }
            EE_ASSERT( IsValid() );
        }

        using TTag::TTag;
        using TTag::operator =;
    };
}