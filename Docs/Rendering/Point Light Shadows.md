# Point Light Shadows

![Point light shadows](Point%20Light%20Shadows/image.png)

Point lights cast shadows in every direction, so their shadow maps has to cover a whole sphere. This is traditionally done with rendering the world 6 times into 6 faces of a cube map texture.

There are several alternatives, like dual paraboloid shadow maps, tetrahedral and octahedral shadow maps, single pass cube map rendering using vendor-specific extensions.

# Octahedral projection

Esoterica uses octahedral shadow maps approach as described by Praun and Hoppe in “Spherical parameterization and remeshing” from 2003 - a sphere is mapped to an octahedron, which is then cut by the 2 midlines and unwrapped into a square.

The parameterization is using the form described by Cigolle et al.: normalize, divide by the L1 norm, and for the back hemisphere reflect into the corners with `( 1 - |p.yx| ) * signNotZero( p )`. 

Each octant has fixed axis signs, which makes the L1 norm linear in world position, which makes the projection well behaved: edges stay straight, depth interpolates exactly and distortion stays within very reasonable texel footprint ratio of 1.30.

![Figure 3 from Cigolle et al.](Point%20Light%20Shadows/image%201.png)

# Single pass rendering

Each point light allocates a single shadow map texture and we draw all 8 octahedral faces in a single draw call. Culling passes for point light shadows use a bounding sphere instead of a view frustum.

Mesh shader dispatch encodes the projection octant into groupID.y and uses the second dispatch dimension to output triangles into each octant. Projection parameters for the current octant are derived from groupID.y, and the shadow map render view has the remaining required parameters.

The main innovation that makes a single pass possible is using custom clip planes in the mesh shader - every triangle is rasterized over the entire square texture and every octant is a triangle; the mesh shader writes 3 custom clip planes representing each octant's half spaces and constrains rasterized triangles within their corresponding octant.

![Single draw call shadow map](Point%20Light%20Shadows/image%202.png)

# Sampling

Sampling this type of shadow map uses a standard PCF filter with clamp sampler.

The inner octant seams are not actual seams and are adjacent to each other, so linear taps work there naturally. Outer seams (square texture boundary) are not - they connect to octants on the opposite side and the clamp sampler handles it.

A potential future improvements is to pad the shadow map by 1 pixel and copy pixels from the opposite side to improve filtering quality.

# References

- Emil Praun, Hugues Hoppe. **Spherical parametrization and remeshing.** ACM Trans. Graphics (SIGGRAPH) 22(3), 2003, [https://hhoppe.com/proj/sphereparam/](https://hhoppe.com/proj/sphereparam/)
- Zina H. Cigolle, Sam Donow, Daniel Evangelakos, Michael Mara, Morgan McGuire, Quirin Meyer. **A Survey of Efficient Representations for Independent Unit Vectors.** Journal of Computer Graphics Techniques, 2014, [https://jcgt.org/published/0003/02/01/](https://jcgt.org/published/0003/02/01/)