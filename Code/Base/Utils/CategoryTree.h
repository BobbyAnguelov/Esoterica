#pragma once
#include "Base/Types/String.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
    template<typename T>
    struct CategoryItem
    {
        CategoryItem( String const& name, T const& data )
            : m_name( name )
            , m_data( data )
        {}

        String                  m_name;
        String                  m_fullPath;
        T                       m_data;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    struct Category
    {
        Category( String const& name, int32_t depth = -1 )
            : m_name( name )
            , m_depth( depth )
        {}

        Category( String&& name, int32_t depth = -1 )
            : m_name( name )
            , m_depth( depth )
        {}

        void Clear()
        {
            m_childCategories.clear();
            m_items.clear();
        }

        bool IsEmpty() const
        {
            return m_childCategories.empty() && m_items.empty();
        }

        bool HasChildCategories() const
        {
            return !m_childCategories.empty();
        }

        bool IsLeafCategory() const
        {
            return m_childCategories.empty();
        }

        bool HasItem( String const& itemName ) const
        {
            for ( auto const& item : m_items )
            {
                if ( item.m_name == itemName )
                {
                    return true;
                }
            }

            return false;
        }

        bool HasItemsThatMatchFilter( TFunction<bool( CategoryItem<T> const& )> const& filter ) const
        {
            for ( auto const& childCategory : m_childCategories )
            {
                if ( childCategory.HasItemsThatMatchFilter( filter ) )
                {
                    return true;
                }
            }

            //-------------------------------------------------------------------------

            for ( auto const& item : m_items )
            {
                if ( filter( item ) )
                {
                    return true;
                }
            }

            //-------------------------------------------------------------------------

            return false;
        }

        void AddItem( CategoryItem<T> const& itemToAdd, bool sortedInsert = true )
        {
            if ( sortedInsert )
            {
                auto Comparision = [] ( CategoryItem<T> const& existingItem, String const& name )
                {
                    return existingItem.m_name < name;
                };

                auto insertionPosition = eastl::lower_bound( m_items.begin(), m_items.end(), itemToAdd.m_name, Comparision );
                CategoryItem<T>* pAddedItem = m_items.insert( insertionPosition, itemToAdd );
                pAddedItem->m_fullPath = m_fullPath + itemToAdd.m_name;

            }
            else
            {
                auto& addedItem = m_items.emplace_back( itemToAdd );
                addedItem.m_fullPath = m_fullPath + addedItem.m_name;
            }
        }

        void RemoveAllItems()
        {
            for ( auto& childCategory : m_childCategories )
            {
                childCategory.RemoveAllItems();
            }

            m_items.clear();
        }

        void RemoveAllEmptyChildCategories()
        {
            for ( auto i = int32_t( m_childCategories.size() ) - 1; i >= 0; i-- )
            {
                m_childCategories[i].RemoveAllEmptyChildCategories();

                if ( m_childCategories[i].IsEmpty() )
                {
                    m_childCategories.erase( m_childCategories.begin() + i );
                }
            }
        }

        void SetUserDataRecursive( int64_t userData )
        {
            m_userData = userData;

            for ( auto& childCategory : m_childCategories )
            {
                childCategory.SetUserDataRecursive( userData );
            }
        }

    public:

        String                      m_name;
        String                      m_fullPath;
        TVector<Category<T>>        m_childCategories;
        TVector<CategoryItem<T>>    m_items;
        int32_t                     m_depth = -1;
        int64_t                     m_userData = 0;
        bool                        m_isCollapsed = false;
    };

    // Category Tree
    //-------------------------------------------------------------------------
    // Allow you to build a simple category tree based on a string path (X/Y/Z) etc...

    template<typename T>
    class CategoryTree
    {
    public:

        // The delimiter used to separate data path elements
        constexpr static char const s_pathDelimiter[] = "/";

    public:

        CategoryTree()
            : m_rootCategory( "" )
        {}

        // Get the root category
        Category<T> const& GetRootCategory() const { return m_rootCategory; }

        // Get the root category
        Category<T>& GetRootCategory() { return m_rootCategory; }

        // Clear the tree
        void Clear() { m_rootCategory.Clear(); }

        // Add a new item
        void AddItem( String const& path, String const& itemName, T const& item, bool sortedInsert = true )
        {
            if ( path.empty() )
            {
                m_rootCategory.AddItem( CategoryItem<T>( itemName, item ), sortedInsert );
            }
            else
            {
                TVector<String> splitPath = SplitPathString( path );
                Category<T>* pFoundCategory = FindOrCreateCategory( m_rootCategory, splitPath, 0 );
                EE_ASSERT( pFoundCategory != nullptr );
                pFoundCategory->AddItem( CategoryItem<T>( itemName, item ), sortedInsert );
            }
        }

        // Add a new unique item - will not add if we have an item with that name already, return true if the item was added
        bool AddUniqueItem( String const& path, String const& itemName, T const& item, bool sortedInsert = true )
        {
            if ( path.empty() )
            {
                if ( !m_rootCategory.HasItem( itemName ) )
                {
                    m_rootCategory.AddItem( CategoryItem<T>( itemName, item ), sortedInsert );
                    return true;
                }
            }
            else
            {
                TVector<String> splitPath = SplitPathString( path );
                Category<T>* pFoundCategory = FindOrCreateCategory( m_rootCategory, splitPath, 0 );
                EE_ASSERT( pFoundCategory != nullptr );
                if ( !pFoundCategory->HasItem( itemName ) )
                {
                    pFoundCategory->AddItem( CategoryItem<T>( itemName, item ), sortedInsert );
                    return true;
                }
            }

            return false;
        }

        Category<T>* AddCategory( String const& path )
        {
            TVector<String> splitPath = SplitPathString( path );
            return FindOrCreateCategory( m_rootCategory, splitPath, 0 );
        }

        // Find a category given a specific path
        Category<T>* FindCategory( String const& path )
        {
            Category<T>* pFoundCategory = nullptr;

            if ( path.empty() )
            {
                pFoundCategory = &m_rootCategory;
                return pFoundCategory;
            }

            //-------------------------------------------------------------------------

            TVector<String> splitPath = SplitPathString( path );
            Category<T>* pCurrentCategory = &m_rootCategory;
            auto pathIter = splitPath.begin();

            while ( pFoundCategory == nullptr && pathIter != splitPath.end() )
            {
                bool foundValidChild = false;
                for ( Category<T>& childCategory : pCurrentCategory->m_childCategories )
                {
                    if ( childCategory.m_name == *pathIter )
                    {
                        pCurrentCategory = &childCategory;
                        foundValidChild = true;
                        break;
                    }
                }

                //-------------------------------------------------------------------------

                if ( foundValidChild )
                {
                    pathIter++;
                    if ( pathIter == splitPath.end() )
                    {
                        pFoundCategory = pCurrentCategory;
                    }
                }
                else
                {
                    break;
                }
            }

            return pFoundCategory;
        }

        // Get parent category for a given category
        Category<T>* GetParentCategory( Category<T> const& category )
        {
            String parentPath;
            size_t const parentDelimiterIdx = category.m_fullPath.rfind( s_pathDelimiter );
            if ( parentDelimiterIdx != String::npos )
            {
                parentPath = category.m_fullPath.substr( 0, parentDelimiterIdx + 1 );
            }

            auto pParentCategory = FindCategory( parentPath );
            return pParentCategory;
        }

        // Flatten out any categories that dont have any items in them. Those categories will be merged into their parent
        void CollapseAllIntermediateCategories()
        {
            CollapseIntermediateCategory( m_rootCategory );
        }

        // Remove all items but keep categories
        void RemoveAllItems()
        {
            m_rootCategory.RemoveAllItems();
        }

        // Remove all empty categories
        void RemoveEmptyCategories()
        {
            m_rootCategory.RemoveAllEmptyChildCategories();
        }

    private:

        void CollapseIntermediateCategory( Category<T> &category )
        {
            for ( auto& childCategory : category.m_childCategories )
            {
                CollapseIntermediateCategory( childCategory );
            }

            // If we have no items and only have a single child category then fold it into us
            if ( category.m_items.empty() )
            {
                if ( category.m_childCategories.size() == 1 )
                {
                    category.m_name.append( s_pathDelimiter );
                    category.m_name.append( category.m_childCategories[0].m_name );

                    for ( int32_t i = 0; i < category.m_childCategories[0].m_childCategories.size(); i++ )
                    {
                        auto const tmpCategory = category.m_childCategories[0].m_childCategories[i]; // Create a copy to avoid invalidating the source when emplacing since this is a child
                        category.m_childCategories.emplace_back( tmpCategory );
                    }

                    for ( int32_t i = 0; i < category.m_childCategories[0].m_items.size(); i++ )
                    {
                        auto const tmpItem = category.m_childCategories[0].m_items[i]; // Create a copy to avoid invalidating the source when emplacing since this is a child
                        category.m_items.emplace_back( tmpItem );
                    }

                    category.m_childCategories.erase( category.m_childCategories.begin() );
                }
            }
        }

        // Split a category path into individual elements
        TVector<String> SplitPathString( String const& pathString )
        {
            EE_ASSERT( !pathString.empty() );
            TVector<String> splitPath;
            StringUtils::Split( pathString, splitPath, s_pathDelimiter );
            return splitPath;
        }

        Category<T>* FindOrCreateCategory( Category<T>& currentCategory, TVector<String> const& path, int32_t currentPathIdx )
        {
            EE_ASSERT( !path.empty() );

            // If we are at the end of the path, this is the category we are looking for
            //-------------------------------------------------------------------------

            if ( path.size() == currentPathIdx )
            {
                return &currentCategory;
            }

            // Search
            //-------------------------------------------------------------------------

            for ( Category<T>& childCategory : currentCategory.m_childCategories )
            {
                if ( childCategory.m_name == path[currentPathIdx] )
                {
                    return FindOrCreateCategory( childCategory, path, currentPathIdx + 1 );
                }
            }

            // Create a new category and continue the search
            auto Comparision = [] ( Category<T> const& category, String const& name )
            {
                return category.m_name < name;
            };

            auto insertionPosition = eastl::lower_bound( currentCategory.m_childCategories.begin(), currentCategory.m_childCategories.end(), path[currentPathIdx], Comparision );
            Category<T>* pNewCategory = currentCategory.m_childCategories.insert( insertionPosition, Category<T>( path[currentPathIdx], currentPathIdx ) );
            pNewCategory->m_fullPath = currentCategory.m_fullPath + path[currentPathIdx] + s_pathDelimiter;
            return FindOrCreateCategory( *pNewCategory, path, currentPathIdx + 1 );
        }

    public:

        Category<T>       m_rootCategory;
    };
}