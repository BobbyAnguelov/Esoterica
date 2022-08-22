#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API GraphDataSet
    {
        EE_SERIALIZE( m_variationID, m_pSkeleton, m_resources );

        friend class AnimationGraphCompiler;
        friend class GraphLoader;
        friend class GraphInstance;

    public:

        inline bool IsValid() const { return m_variationID.IsValid() && m_pSkeleton.IsLoaded(); }

        inline Skeleton const* GetSkeleton() const { return m_pSkeleton.GetPtr(); }

        template<typename T>
        inline T const* GetResource( int16_t const& slotIdx ) const
        {
            EE_ASSERT( slotIdx >= 0 && slotIdx < m_resources.size() );
            if ( m_resources[slotIdx].IsValid() )
            {
                return TResourcePtr<T>( m_resources[slotIdx] ).GetPtr();
            }

            return nullptr;
        }

    private:

        StringID                                    m_variationID;
        TResourcePtr<Skeleton>                      m_pSkeleton = nullptr;
        TVector<Resource::ResourcePtr>              m_resources;
    };
}