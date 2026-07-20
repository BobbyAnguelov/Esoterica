# Meshes

This page covers the vertex format, compression scheme, and how meshes connect to instances and materials.

## Vertex Compression

We implement lossy (98% accuracy) mesh compression by exploiting mesh clustering and vertex spatial location.

Vertex positions are encoded as 16-bit unsigned offsets from a per-cluster anchor with a shared exponent — the same encoding as [DXR2 COMPRESSED1](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing2.html#compressed1-position-encoding), so vertex data is directly compatible with DXR2 ray tracing without conversion.

Normals are stored as 16-bit signed normalized values.

Position and normal compress into 6 16-bit values, bringing the static mesh vertex size down to 32 bytes and skeletal mesh vertex size to 64 bytes:

```cpp
struct MeshCluster final
{
    uint16_t m_boundingSphere[4] = {};       // W is unused
    uint32_t m_vertexOffset = 0;
    uint32_t m_triangleOffset = 0;
    int32_t  m_anchorX : 24 = 0;
    uint32_t m_numVertices : 8 = 0;          // only 6 bits needed
    int32_t  m_anchorY : 24 = 0;
    uint32_t m_numTriangles : 8 = 0;         // only 7 bits needed
    int32_t  m_anchorZ : 24 = 0;
    int32_t  m_sharedExponent : 8 = 0;
    float    m_boundingSphereRadius = 0.0F;  // full 32 bits is wasteful, candidate for future refactor
};

struct StaticMeshVertex final
{
    uint16_t m_compressedPositionX = 0;
    uint16_t m_compressedPositionY = 0;
    uint16_t m_compressedPositionZ = 0;
    int16_t  m_compressedNormalX = 0;
    int16_t  m_compressedNormalY = 0;
    int16_t  m_compressedNormalZ = 0;
    Float2   m_uv0 = Float2::Zero;
    Float2   m_uv1 = Float2::Zero;
    uint32_t m_packedColor = 0;

    void Initialize( Int3 compressedPosition, Float3 normal );
    Float3 GetPosition( Int3 anchor, int32_t exponent ) const;
    Float3 GetNormal() const;
};

struct SkeletalMeshVertex final
{
    StaticMeshVertex m_vertex;
    Int4             m_boneIndices;
    Float4           m_boneWeights;
};
```

Decompression is only practical with mesh shaders. We dispatch 1 group per cluster and 1 thread per vertex.

Cluster building and vertex compression happen in `MeshBuilder::BuildAndAppendClusters`.

## Custom Vertex Attributes

We plan to support arbitrary custom vertex attributes — varying numbers of UV channels, choosing between 4 or 8 bone weights, and similar.

**This is not implemented yet but is high on the todo list.**