#include "RenderVertexFormats.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static uint32_t const g_dataTypeSizes[] =
    {
        0,

        1,
        2,
        4,

        1,
        2,
        4,

        4,
        8,
        12,
        16,

        4,
        8,
        12,
        16,

        2,
        4,
        8,

        4,
        8,
        12,
        16,

        4
    };

    static_assert( sizeof( g_dataTypeSizes ) / sizeof( uint32_t ) == (uint32_t) DataFormat::Count, "Mismatched data type and size arrays" );

    uint32_t GetDataTypeFormatByteSize( DataFormat format )
    {
        uint32_t const formatIdx = (uint32_t) format;
        EE_ASSERT( formatIdx < (uint32_t) DataFormat::Count );
        uint32_t const size = g_dataTypeSizes[formatIdx];
        return size;
    }

    //-------------------------------------------------------------------------

    void VertexLayoutDescriptor::CalculateByteSize()
    {
        m_byteSize = 0;
        for ( auto const& vertexElementDesc : m_elementDescriptors )
        {
            m_byteSize += GetDataTypeFormatByteSize( vertexElementDesc.m_format );
        }
    }

    //-------------------------------------------------------------------------

    namespace VertexLayoutRegistry
    {
        VertexLayoutDescriptor GetDescriptorForFormat( VertexFormat format )
        {
            EE_ASSERT( format != VertexFormat::Unknown );

            VertexLayoutDescriptor layoutDesc;

            if ( format == VertexFormat::StaticMesh )
            {
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Position, DataFormat::Float_R32G32B32A32, 0, 0 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Normal, DataFormat::Float_R32G32B32A32, 0, 16 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::TexCoord, DataFormat::Float_R32G32, 0, 32 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::TexCoord, DataFormat::Float_R32G32, 1, 40 ) );

            }
            else if ( format == VertexFormat::SkeletalMesh )
            {
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Position, DataFormat::Float_R32G32B32A32, 0, 0 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Normal, DataFormat::Float_R32G32B32A32, 0, 16 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::TexCoord, DataFormat::Float_R32G32, 0, 32 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::TexCoord, DataFormat::Float_R32G32, 1, 40 ) );

                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::BlendIndex, DataFormat::SInt_R32G32B32A32, 0, 48 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::BlendWeight, DataFormat::Float_R32G32B32A32, 0, 64 ) );
            }

            //-------------------------------------------------------------------------

            layoutDesc.CalculateByteSize();
            return layoutDesc;
        }
    }
}