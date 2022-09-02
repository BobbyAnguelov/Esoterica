#pragma once
#include "System/Types/String.h"
#include "System/Types/Arrays.h"
#include "System/Types/Function.h"

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
        T                       m_data;
    };

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

    public:

        String                      m_name;
        TVector<Category<T>>        m_childCategories;
        TVector<CategoryItem<T>>    m_items;
        int32_t                     m_depth = -1;
    };

    // Category Tree
    //-------------------------------------------------------------------------
    // Allow you to build a simple category tree based on a string path (X/Y/Z) etc...

    template<typename T>
    class CategoryTree
    {
    public:

        CategoryTree()
            : m_rootCategory( "" )
        {}

        Category<T> const& GetRootCategory() const { return m_rootCategory; }

        // Add a new item
        void AddItem( String const& path, String const& itemName, T const& item )
        {
            if ( path.empty() )
            {
                m_rootCategory.m_items.emplace_back( CategoryItem<T>( itemName, item ) );
            }
            else
            {
                TVector<String> splitPath = SplitPathString( path );
                Category<T>* pFoundCategory = FindOrCreateCategory( m_rootCategory, splitPath, 0 );
                EE_ASSERT( pFoundCategory != nullptr );

                auto Comparision = [] ( CategoryItem<T> const& item, String const& name )
                {
                    return item.m_name < name;
                };

                auto insertionPosition = eastl::lower_bound( pFoundCategory->m_items.begin(), pFoundCategory->m_items.end(), itemName, Comparision );
                pFoundCategory->m_items.insert( insertionPosition, CategoryItem<T>( itemName, item ) );
            }
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
                for ( auto pChildCategory : pCurrentCategory->m_childCategories )
                {
                    if ( pChildCategory->m_name == *pathIter )
                    {
                        pCurrentCategory = pChildCategory;
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

    private:

        // Split a category path into individual elements
        TVector<String> SplitPathString( String const& pathString )
        {
            EE_ASSERT( !pathString.empty() );
            TVector<String> splitPath;
            StringUtils::Split( pathString, splitPath, "/" );
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
            auto newCategoryIter = currentCategory.m_childCategories.insert( insertionPosition, Category<T>( path[currentPathIdx], currentPathIdx ) );
            return FindOrCreateCategory( *newCategoryIter, path, currentPathIdx + 1 );
        }

    public:

        Category<T>       m_rootCategory;
    };
}