#pragma once

#include "System/_Module/API.h"
#include "RenderAPI.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/Types/Arrays.h"
#include "System/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_SYSTEM_API uint32_t GetDataTypeFormatByteSize( DataFormat format );

    //-------------------------------------------------------------------------

    enum class VertexFormat
    {
        Unknown = 0,
        None,
        StaticMesh,
        SkeletalMesh,
    };

    // CPU format for the static mesh vertex - this is what the mesh compiler fills the vertex data array with
    struct StaticMeshVertex
    {
        Float4  m_position;
        Float4  m_normal;
        Float2  m_UV0;
        Float2  m_UV1;
    };

    // CPU format for the skeletal mesh vertex - this is what the mesh compiler fills the vertex data array with
    struct SkeletalMeshVertex : public StaticMeshVertex
    {
        Int4    m_boneIndices;
        Float4  m_boneWeights;
    };

    //-------------------------------------------------------------------------

    struct EE_SYSTEM_API VertexLayoutDescriptor
    {
        EE_SERIALIZE( m_elementDescriptors, m_byteSize );

        struct ElementDescriptor
        {
            EE_SERIALIZE( m_semantic, m_format, m_semanticIndex, m_offset );

            ElementDescriptor() = default;

            ElementDescriptor( DataSemantic semantic, DataFormat format, uint16_t semanticIndex, uint16_t offset ) : m_semantic( semantic )
                , m_format( format )
                , m_semanticIndex( semanticIndex )
                , m_offset( offset )
            {}

            DataSemantic        m_semantic = DataSemantic::None;
            DataFormat      m_format = DataFormat::Unknown;
            uint16_t              m_semanticIndex = 0;
            uint16_t              m_offset = 0;
        };

    public:

        inline bool IsValid() const { return m_byteSize > 0 && !m_elementDescriptors.empty(); }
        void CalculateByteSize();

    public:

        TInlineVector<ElementDescriptor, 6>     m_elementDescriptors;
        uint32_t                                  m_byteSize = 0;             // The total byte size per vertex
    };

    //-------------------------------------------------------------------------

    namespace VertexLayoutRegistry
    {
        EE_SYSTEM_API VertexLayoutDescriptor GetDescriptorForFormat( VertexFormat format );
    }
}