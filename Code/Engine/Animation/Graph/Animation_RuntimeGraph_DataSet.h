#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API GraphDataSet
    {
        EE_SERIALIZE( m_variationID, m_skeleton, m_resources );

        friend class AnimationGraphCompiler;
        friend class GraphLoader;
        friend class GraphInstance;

    public:

        inline bool IsValid() const { return m_variationID.IsValid() && m_skeleton.IsLoaded(); }

        inline Skeleton const* GetSkeleton() const { return m_skeleton.GetPtr(); }

        template<typename T>
        inline T const* GetResource( int16_t const& slotIdx ) const
        {
            EE_ASSERT( slotIdx >= 0 && slotIdx < m_resources.size() );
            if ( m_resources[slotIdx].IsSet() )
            {
                return TResourcePtr<T>( m_resources[slotIdx] ).GetPtr();
            }

            return nullptr;
        }

    private:

        StringID                                    m_variationID;
        TResourcePtr<Skeleton>                      m_skeleton = nullptr;
        TVector<Resource::ResourcePtr>              m_resources;
    };
}