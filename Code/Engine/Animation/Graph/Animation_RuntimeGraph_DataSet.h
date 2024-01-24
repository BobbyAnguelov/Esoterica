#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Types/HashMap.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    using ResourceLUT = THashMap<uint32_t, Resource::ResourcePtr>;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GraphDataSet
    {
        EE_SERIALIZE( m_variationID, m_skeleton, m_resources );

        friend class AnimationGraphCompiler;
        friend class GraphLoader;
        friend class GraphInstance;

    public:

        inline bool IsValid() const { return m_variationID.IsValid() && m_skeleton.IsLoaded(); }

        inline StringID GetVariationID() const { return m_variationID; }

        inline Skeleton const* GetPrimarySkeleton() const { return m_skeleton.GetPtr(); }

        template<typename T>
        inline T const* GetResource( int16_t slotIdx ) const
        {
            EE_ASSERT( slotIdx >= 0 && slotIdx < m_resources.size() );
            if ( m_resources[slotIdx].IsSet() )
            {
                return TResourcePtr<T>( m_resources[slotIdx] ).GetPtr();
            }

            return nullptr;
        }

        // Get the flattened resource lookup table
        ResourceLUT const& GetResourceLookupTable() const { return m_resourceLUT; }

    private:

        StringID                                        m_variationID;
        TResourcePtr<Skeleton>                          m_skeleton = nullptr;
        TVector<Resource::ResourcePtr>                  m_resources;

        // Used to lookup all resource in this dataset as well any child datasets.
        // This is basically a flattened list of all resources references by this dataset.
        // Note: This is generated at install time
        ResourceLUT                                     m_resourceLUT;
    };
}