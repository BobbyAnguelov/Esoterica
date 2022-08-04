#pragma once
#include "Arrays.h"
#include "HashMap.h"

//-------------------------------------------------------------------------
// ID Vector
//-------------------------------------------------------------------------
// Utility container for when we want to iterate over a contiguous set of items but also have fast lookups
// Expects contained types to provide a GetID() function that returns IDType
// e.g 
//      struct Foo
//      {
//          uint32_t GetID() const;
//      };
//      
//      TIDVector<uint32_t, Foo> m_foos;

struct Foo
{
    uint32_t GetID() const { return 0; }
};

namespace EE
{
    template<typename IDType, typename ItemType>
    class TIDVector
    {
    public:

        // Flat array access
        //-------------------------------------------------------------------------

        TVector<ItemType> const& GetVector() const { return m_vector; }
        ItemType& operator[]( int32_t idx ) { return m_vector[idx]; }
        int32_t size() const { return (int32_t) m_vector.size(); }
        bool empty() const { return m_vector.empty(); }

        typename TVector<ItemType>::iterator begin() { return m_vector.begin(); }
        typename TVector<ItemType>::iterator end() { return m_vector.end(); }

        typename TVector<ItemType>::const_iterator begin() const { return m_vector.begin(); }
        typename TVector<ItemType>::const_iterator end() const { return m_vector.end(); }

        // ID management Insertion / Deletion / Search
        //-------------------------------------------------------------------------

        // Check if we have an item for a given ID
        bool HasItemForID( IDType const& ID ) const
        {
            return m_indexMap.find( ID ) != m_indexMap.end();
        }

        // Add a new item
        ItemType* Add( ItemType const& item )
        {
            IDType const ID = GetItemID( std::is_pointer<ItemType>(), item );
            EE_ASSERT( m_indexMap.find( ID ) == m_indexMap.end() );
            int32_t const itemIdx = (int32_t) m_vector.size();
            m_vector.emplace_back( item );
            m_indexMap.insert( TPair<IDType, int32_t>( ID, itemIdx ) );
            return &m_vector[itemIdx];
        }

        // Add a new item
        ItemType* Add( ItemType const&& item )
        {
            IDType const ID = GetItemID( std::is_pointer<ItemType>(), item );
            EE_ASSERT( m_indexMap.find( ID ) == m_indexMap.end() );
            int32_t const itemIdx = (int32_t) m_vector.size();
            m_vector.emplace_back( eastl::forward<ItemType const>( item ) );
            m_indexMap.insert( TPair<IDType, int32_t>( ID, itemIdx ) );
            return &m_vector[itemIdx];
        }

        // Emplace a new item
        template<class... Args>
        ItemType* Emplace( IDType const& ID, Args&&... args )
        {
            EE_ASSERT( m_indexMap.find( ID ) == m_indexMap.end() );
            int32_t const itemIdx = (int32_t) m_vector.size();
            m_vector.emplace_back( eastl::forward<Args>( args )... );
            EE_ASSERT( GetLastElementID( std::is_pointer<ItemType>() ) == ID );
            m_indexMap.insert( TPair<IDType, int32_t>( ID, itemIdx ) );
            return &m_vector[itemIdx];
        }

        // Remove item
        void Remove( IDType const& ID )
        {
            auto foundIter = m_indexMap.find( ID );
            EE_ASSERT( foundIter != m_indexMap.end() );

            // Update the index for the last element if that exists
            if ( m_vector.size() > 1 )
            {
                IDType lastItemID = GetLastElementID( std::is_pointer<ItemType>() );
                auto foundLastItemIter = m_indexMap.find( lastItemID );
                foundLastItemIter->second = foundIter->second;
            }

            // Erase by swapping with the last element and popping
            m_vector.erase_unsorted( m_vector.begin() + foundIter->second );
            m_indexMap.erase( foundIter );
        }

        // Try to find an item matching the ID, returns nullptr if no item for this ID exists
        ItemType* FindItem( IDType const& ID )
        {
            ItemType* pFound = nullptr;

            auto foundIter = m_indexMap.find( ID );
            if ( foundIter != m_indexMap.end() )
            {
                pFound = &m_vector[foundIter->second];
            }

            return pFound;
        }

        // Try to find an item matching the ID, if we dont find one create a default item and return a ptr to it
        // This function will emplace a new type if it doesnt exist using the supplied ctor arguments
        template<class... Args>
        ItemType* FindOrAdd( IDType const& ID, Args&&... args )
        {
            auto pFoundItem = FindItem( ID );
            if ( pFoundItem == nullptr )
            {
                pFoundItem = Emplace( ID, eastl::forward<Args>( args )... );
            }

            return pFoundItem;
        }

        // Returns the item for a specified ID, expect the item to exists! Will crash if the item does not!
        ItemType* Get( IDType const& ID )
        {
            auto pFoundItem = FindItem( ID );
            EE_ASSERT( pFoundItem != nullptr );
            return pFoundItem;
        }

    private:

        EE_FORCE_INLINE IDType GetItemID( std::true_type, ItemType const& pItem ) const
        {
            return pItem->GetID();
        }

        EE_FORCE_INLINE IDType GetItemID( std::false_type, ItemType const& item ) const
        {
            return item.GetID();
        }

        EE_FORCE_INLINE IDType GetLastElementID( std::true_type ) const
        {
            EE_ASSERT( !m_vector.empty() );
            return m_vector.back()->GetID();
        }

        EE_FORCE_INLINE IDType GetLastElementID( std::false_type ) const
        {
            EE_ASSERT( !m_vector.empty() );
            return m_vector.back().GetID();
        }

    private:

        TVector<ItemType>                   m_vector;
        THashMap<IDType, int32_t>           m_indexMap; // A mapping between the ID type and the item index in the flat array
    };
}