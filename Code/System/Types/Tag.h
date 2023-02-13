#pragma once
#include "System/Types/StringID.h"
#include "System/Serialization/BinarySerialization.h"
#include "Arrays.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------
// General Tag
//-------------------------------------------------------------------------
// Categorized 4-Level 128bit tag consisting of 4 StringIDs
// Categories must be sequentially set for the tag to be valid, you cannot have an invalid ID between valid IDs.

namespace EE
{
    class EE_SYSTEM_API Tag
    {
        EE_SERIALIZE( m_c );

    public:

        using QueryResults = TInlineVector<Tag, 5>;

        // Utility Functions
        //-------------------------------------------------------------------------

        // Create a tag from a string in the format "C0.C1.C2.C3"
        static Tag FromTagFormatString( String const& tagFormatString );

        // Does this container have a tag that exactly matches the specified tag
        template<typename T>
        static bool HasExactlyMatchingTag( T const& tagContainer, Tag const& tagToMatch )
        {
            for ( Tag const& tag : tagContainer )
            {
                if ( tag == tagToMatch )
                {
                    return true;
                }
            }

            return false;
        }

        // Ensure that the supplied container doesnt have any duplicate tags
        template<typename T>
        static void EnsureUniqueTags( T& tagContainer )
        {
            auto SortPredicate = [] ( Tag const& a, Tag const& b )
            {
                for ( int32_t i = 0; i < 4; i++ )
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

            eastl::sort( tagContainer.begin(), tagContainer.end(), SortPredicate );
            auto lastIter = eastl::unique( tagContainer.begin(), tagContainer.end() );
            tagContainer.erase( lastIter, tagContainer.end() );
        }

        // Get all the child tags for a given parent in this container, returns true if any children were found.
        template<typename T>
        static bool FindAllChildTags( T const& tagContainer, Tag const& parentTag, QueryResults& outResults )
        {
            bool childrenWereFound = false;
            outResults.clear();

            for ( Tag const& tag : tagContainer )
            {
                if ( tag.IsChildOf( parentTag ) )
                {
                    outResults.emplace_back( tag );
                    childrenWereFound = true;
                }
            }

            return childrenWereFound;
        }

        // Does this container has any tags that are children of the supplied tag
        template<typename T>
        static bool HasAnyChildTags( T const& tagContainer, Tag const& parentTag )
        {
            for ( Tag const& tag : tagContainer )
            {
                if ( tag.IsChildOf( parentTag ) )
                {
                    return true;
                }
            }

            return false;
        }

    public:

        Tag()
        {
            m_c[0] = m_c[1] = m_c[2] = m_c[3] = StringID();
        }

        explicit Tag( char const* p0, char const* p1 = nullptr, char const* p2 = nullptr, char const* p3 = nullptr );

        explicit Tag( StringID s0, StringID s1 = StringID(), StringID s2 = StringID(), StringID s3 = StringID() );

        // Is this tag valid? Categories must be sequentially set for the tag to be valid, you cannot have an invalid ID between valid IDs.
        bool IsValid() const;

        // Get the tag as a string in the following format: "C0.C1.C2.C3"
        String ToString() const;

        // Get the current tag depth (i.e. how many levels have been set)
        int32_t GetDepth() const;

        // Accessors
        //-------------------------------------------------------------------------

        // Get the parent tag, i.e. the tag with one less category set
        Tag GetParentTag() const;

        // Get specific category at the specified level
        StringID GetValueAtLevel( int32_t level );

        // Queries and Comparisons
        //-------------------------------------------------------------------------

        // Are these tags EXACTLY equal
        inline bool operator==( Tag const& rhs ) const
        {
            return m_c[0] == rhs.m_c[0] && m_c[1] == rhs.m_c[1] && m_c[2] == rhs.m_c[2] && m_c[3] == rhs.m_c[3];
        }

        // Are these tags different
        inline bool operator!=( Tag const& rhs ) const
        {
            return m_c[0] != rhs.m_c[0] || m_c[1] != rhs.m_c[1] || m_c[2] != rhs.m_c[2] || m_c[3] != rhs.m_c[3];
        }

        // Is this tag a parent of the specified tag? e.g., A.B is a parent to A.B.C and A.B.Q
        bool IsParentOf( Tag const& childTag ) const;

        // Is this tag a child of the specified tag? e.g., A.B.C derives from A and from A.B
        bool IsChildOf( Tag const& parentTag ) const;

    private:

        StringID m_c[4];
    };
}