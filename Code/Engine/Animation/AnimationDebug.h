#pragma once

#include "Engine/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    //-------------------------------------------------------------------------
    // Debug Path
    //-------------------------------------------------------------------------
    // Used to track sources of operation using generic IDs, how these paths are resolved is up to the specific use case
    // TODO: should this be generalized into a base concept?

    struct EE_ENGINE_API DebugPath
    {
        struct Element
        {
            Element() = default;
            Element( int64_t itemID, InlineString&& pathString ) : m_itemID( itemID ),  m_pathString( pathString ) {}

        public:

            int64_t                     m_itemID = InvalidIndex;
            InlineString                m_pathString;
        };

    public:

        inline bool IsValid() const { return !m_path.empty(); }

        inline int32_t GetNumElements() const { return (int32_t) m_path.size(); }

        InlineString GetFlattenedPath() const;

        void PopFront( size_t numElements );

        //-------------------------------------------------------------------------

        inline size_t size() const { return m_path.size(); }
        EE_FORCE_INLINE TInlineVector<Element, 10>::iterator begin() { return m_path.begin(); }
        EE_FORCE_INLINE TInlineVector<Element, 10>::iterator end() { return m_path.end(); }
        EE_FORCE_INLINE TInlineVector<Element, 10>::const_iterator begin() const { return m_path.begin(); }
        EE_FORCE_INLINE TInlineVector<Element, 10>::const_iterator end() const { return m_path.end(); }
        EE_FORCE_INLINE Element& operator[]( uint32_t i ) { return m_path[i]; }
        EE_FORCE_INLINE Element const& operator[]( uint32_t i ) const { return m_path[i]; }

    public:

        TInlineVector<Element, 10>      m_path;
    };

    //-------------------------------------------------------------------------
    // Debug Path Tracker
    //-------------------------------------------------------------------------

    struct DebugPathTracker
    {
        ~DebugPathTracker();

        void Clear();

        // Do we have a base path set
        bool HasBasePath() const { return !m_basePath.empty(); }

        // Add a new path ID to the base path
        void PushBasePath( int64_t itemID ) { m_basePath.emplace_back( itemID ); }

        // Pop a path ID from the base path
        void PopBasePath() { EE_ASSERT( !m_basePath.empty() ); m_basePath.pop_back(); }

        // Add a new item path
        void AddTrackedPath( int64_t itemID )
        {
            auto& newItemPath = m_itemPaths.emplace_back( m_basePath );
            newItemPath.emplace_back( itemID );
        }

        // Remove a tracked item path
        void RemoveTrackedPath( int32_t itemIdx )
        {
            EE_ASSERT( itemIdx >= 0 && itemIdx < m_itemPaths.size() );
            m_itemPaths.erase( m_itemPaths.begin() + itemIdx );
        }

    public:

        TInlineVector<int64_t, 5>                   m_basePath;
        TVector<TInlineVector<int64_t, 5>>          m_itemPaths;
    };
}
#endif