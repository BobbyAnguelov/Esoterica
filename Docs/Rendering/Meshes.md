# Meshes

This page covers the vertex format, compression scheme, and how meshes connect to instances and materials.

## Vertex Compression

We implement lossy (98% accuracy) mesh compression by exploiting mesh clustering and vertex spatial location.

Vertex positions are encoded as variable-width ( up to 16 bit per axis, 10 bits on average ) offsets from a per-cluster anchor with a mesh-wide shared exponent -- the same encoding as [DXR2 COMPRESSED1](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing2.html#compressed1-position-encoding), so vertex data is directly compatible with DXR2 ray tracing without conversion.

Normals are stored as 16-bit signed normalized values.

We support up to 3 skinning attributes per vertex, each with 4 bone influences (up to 12 influences total). Each skinning attribute adds 32 bytes of data per-vertex.

We support up to 3 UV and vertex color channels per vertex, each of them adds 8 and 4 bytes of data per-vertex.

Vertex data is deinterleaved per attribute: the variable-width position bitstream comes first, then tightly packed arrays per attribute (normals, UVs, colors, skinning), then the triangle bitstream.

```cpp
struct MeshCluster final                // 32 bytes
{
    int32_t  m_anchorX : 24;
    uint32_t m_numVertices : 6;         // count - 1, max 64
    uint32_t m_numSkinningAttributes : 2;

    int32_t  m_anchorY : 24;
    uint32_t m_numTriangles : 6;        // count - 1, max 64
    uint32_t m_numTextureCoordinateAttributes : 2;

    int32_t  m_anchorZ : 24;
    int32_t  m_sharedExponent : 8;

    uint32_t m_numColorAttributes : 2;
    uint32_t m_numPositionBitsX : 4;    // count - 1, max 16
    uint32_t m_numPositionBitsY : 4;
    uint32_t m_numPositionBitsZ : 4;
    uint32_t m_reserved : 18;

    uint16_t m_aabb[6];                 // anchor-relative AABB min/max
    uint32_t m_dataOffsetIn8ByteBlocks;
};

struct VertexNormalAttribute final
{
    int16_t                         m_compressedNormalX = 0;
    int16_t                         m_compressedNormalY = 0;
    int16_t                         m_compressedNormalZ = 0;
};

using TextureCoordinateAttribute = Float2;
using VertexColorAttribute = uint32_t;

struct SkinningAttribute final
{
    Int4             m_boneIndices;
    Float4           m_boneWeights;
};
```

Decompression is only practical with mesh shaders. We dispatch 1 group per cluster and 1 thread per vertex.

Cluster building and vertex compression happen in `GeometryBuilder::BuildClusters`.

## Custom Vertex Attributes

Custom vertex attributes can be encoded into additional UV channels and vertex color channels. Up to 3 UV channels and up to 3 vertex color channels are supported.