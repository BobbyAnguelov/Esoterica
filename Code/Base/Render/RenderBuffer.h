#pragma once

#include "Base/_Module/API.h"
#include "Base/Render/RenderAPI.h"
#include "Base/Render/RenderVertexFormats.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_BASE_API RenderBuffer
    {
        friend class RenderDevice;

        EE_SERIALIZE( m_ID, m_slot, m_byteSize, m_byteStride, m_usage, m_type );

    public:

        enum class Type
        {
            Unknown = 0,
            Vertex,
            Index,
            Constant,
        };

        enum class Usage
        {
            GPU_only,
            CPU_and_GPU,
        };

    public:

        RenderBuffer() = default;
        RenderBuffer( RenderBuffer const& ) = default;
        ~RenderBuffer() { EE_ASSERT( !m_resourceHandle.IsValid() ); }

        RenderBuffer& operator=( RenderBuffer const& ) = default;

        inline bool IsValid() const { return m_resourceHandle.IsValid(); }

        BufferHandle const& GetResourceHandle() const { return m_resourceHandle; }
        inline uint32_t GetNumElements() const { return m_byteSize / m_byteStride; }

    public:

        uint32_t                m_ID;
        uint32_t                m_slot = 0;
        uint32_t                m_byteSize = 0;
        uint32_t                m_byteStride = 0;
        Usage                   m_usage = Usage::GPU_only;
        Type                    m_type = Type::Unknown;

    protected:

        BufferHandle            m_resourceHandle;
    };

    //-------------------------------------------------------------------------

    class EE_BASE_API VertexBuffer : public RenderBuffer
    {
        friend class RenderDevice;
        EE_SERIALIZE( EE_SERIALIZE_BASE( RenderBuffer ), m_vertexFormat );

    public:

        VertexBuffer() : RenderBuffer() { m_type = Type::Vertex; }

    public:

        VertexFormat            m_vertexFormat = VertexFormat::StaticMesh;
    };
}