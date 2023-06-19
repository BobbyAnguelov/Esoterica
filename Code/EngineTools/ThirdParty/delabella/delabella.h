/*
DELABELLA - Delaunay triangulation library
Copyright (C) 2018-2022 GUMIX - Marcin Sokalski
*/

#ifndef DELABELLA_H
#define DELABELLA_H

template<typename T = double, typename I = int>
struct IDelaBella2
{
	struct Vertex;
	struct Simplex;
	struct Iterator;

	struct Vertex
	{
		Vertex* next; // next in internal / boundary set of vertices
		Simplex* sew; // one of triangles sharing this vertex
		T x, y; // coordinates (input copy)
		I i; // index of original point

		inline const Simplex* StartIterator(Iterator* it/*not_null*/) const;
	};

	struct Simplex
	{
		Vertex* v[3];  // 3 vertices spanning this triangle
		Simplex* f[3]; // 3 adjacent faces, f[i] is at the edge opposite to vertex v[i]
		Simplex* next; // next triangle (of delaunay set or hull set)
		I index;       // list index
		
		unsigned char flags; 

		bool IsDelaunay() const
		{
			return !(flags & 0b10000000);
		}

		bool IsInterior(int at) const
		{
			return flags & 0b01000000;
		}

		bool IsEdgeFixed(int at) const
		{
			return flags & (1 << at);
		}

		inline const Simplex* StartIterator(Iterator* it/*not_null*/, int around/*0,1,2*/) const;
	};

	struct Iterator
	{
		const Simplex* current;
		int around;

		const Simplex* Next()
		{
			int pivot = around + 1;
			if (pivot == 3)
				pivot = 0;

			Simplex* next = current->f[pivot];
			Vertex* v = current->v[around];

			if (next->v[0] == v)
				around = 0;
			else
			if (next->v[1] == v)
				around = 1;
			else
				around = 2;

			current = next;
			return current;
		}

		const Simplex* Prev()
		{
			int pivot = around - 1;
			if (pivot == -1)
				pivot = 2;

			Simplex* prev = current->f[pivot];
			Vertex* v = current->v[around];

			if (prev->v[0] == v)
				around = 0;
			else
			if (prev->v[1] == v)
				around = 1;
			else
				around = 2;

			current = prev;
			return current;
		}
	};

	static IDelaBella2<T,I>* Create();
	virtual void Destroy() = 0;

	virtual void SetErrLog(int(*proc)(void* stream, const char* fmt, ...), void* stream) = 0;

	// return 0: no output 
	// negative: all points are colinear, output hull vertices form colinear segment list, no triangles on output
	// positive: output hull vertices form counter-clockwise ordered segment contour, delaunay and hull triangles are available
	// if 'y' pointer is null, y coords are treated to be located immediately after every x
	// if advance_bytes is less than 2*sizeof coordinate type, it is treated as 2*sizeof coordinate type  
	virtual I Triangulate(I points, const T* x, const T* y = 0, size_t advance_bytes = 0, I stop = -1) = 0;

	// num of points passed to last call to Triangulate()
	virtual I GetNumInputPoints() const = 0;

	// num of indices returned from last call to Triangulate()
	virtual I GetNumOutputIndices() const = 0;

	// num of hull faces (non delaunay triangles)
	virtual I GetNumOutputHullFaces() const = 0;

	// num of boundary vertices
	virtual I GetNumBoundaryVerts() const = 0;

	// num of internal vertices
	virtual I GetNumInternalVerts() const = 0;

	// when called right after Triangulate() / Constrain() / FloodFill() it returns number of triangles,
	// but if called after Polygonize() it returns number of polygons
	virtual I GetNumPolygons() const = 0;

	virtual const Simplex* GetFirstDelaunaySimplex() const = 0; // valid only if Triangulate() > 0
	virtual const Simplex* GetFirstHullSimplex() const = 0; // valid only if Triangulate() > 0
	virtual const Vertex*  GetFirstBoundaryVertex() const = 0; // if Triangulate() < 0 it is list, otherwise closed contour! 
	virtual const Vertex*  GetFirstInternalVertex() const = 0; // valid only if Triangulate() > 0

	// given input point index, returns corresponding vertex pointer 
	virtual const Vertex*  GetVertexByIndex(I i) const = 0;

	// insert constraint edges into triangulation, valid only if Triangulate() > 0
	virtual I ConstrainEdges(I edges, const I* pa, const I* pb, size_t advance_bytes) = 0;

	// assigns interior / exterior flags to all faces, valid only if Triangulate() > 0
	// returns number of 'land' faces (they start at GetFirstDelaunaySimplex)
	// optionally <exterior> pointer is set to the first 'sea' face
	// if invert is set, outer-most faces will become 'land' (instead of 'sea')
	virtual I FloodFill(bool invert, const Simplex** exterior = 0) = 0;

	// groups adjacent faces, not separated by constraint edges, built on concyclic vertices into polygons
	// first 3 vertices of a polygon are all 3 vertices of first face Simplex::v[0], v[1], v[2]
	// every next face in polygon defines additional 1 polygon vertex at its Simplex::v[0]
	// usefull as preprocessing step before genereating voronoi diagrams 
	// and as unification step before comparing 2 or more triangulations
	// valid only if Triangulate() > 0
	virtual I Polygonize(const Simplex* poly[/*GetNumOutputIndices()/3*/] = 0) = 0; 

	// GenVoronoiDiagramVerts(), valid only if Triangulate() > 0
	// it makes sense to call it prior to constraining only
	// generates VD vertices (for use with VD edges or VD polys indices)
	// <x>,<y> can be null if only number of vertices is needed
	// assuming:
	//   N = GetNumBoundaryVerts()
	//   M = GetNumInternalVerts()
	//   P = GetNumPolygons()
	//   V = P + N
	// <x>,<y> array must be (at least) V elements long
	// first P <x>,<y> elements will contain internal points (divisor W=1)
	// next N <x>,<y> elements will contain edge normals (divisor W=0)
	// function returns number vertices filled (V) on success, otherwise 0
	virtual I GenVoronoiDiagramVerts(T* x, T* y, size_t advance_bytes = 0) const = 0;


	// GenVoronoiDiagramEdges(), valid only if Triangulate() > 0
	// it makes sense to call it prior to constraining only
	// generates unidirected VD edges (without ones in opposite direction)
	// assuming:
	//   N = GetNumBoundaryVerts()
	//   M = GetNumInternalVerts()
	//   P = GetNumPolygons()
	//   I = 2 * (N + M + P - 1)
	// <indices> must be (at least) I elements long or must be null
	// every pair of consecutive values in <indices> represent VD edge
	// there is no guaranteed correspondence between edges order and other data
	// function returns number of indices filled (I) on success, otherwise 0
	virtual I GenVoronoiDiagramEdges(I* indices, size_t advance_bytes = 0) const = 0;

	// GenVoronoiDiagramPolys() valid only if Triangulate() > 0
	// it makes sense to call it prior to constraining only
	// generates VD polygons
	// assuming:
	//   N = GetNumBoundaryVerts()
	//   M = GetNumInternalVerts()
	//   P = GetNumPolygons()
	//   I = 3 * (N + M) + 2 * (P - 1) + N
	// <indices> must be (at least) I elements long or must be null
	// first M polys in <indices> represent closed VD cells, thay are in order
	// and corresponding to vertices from GetFirstInternalVertex() -> Vertex::next list
	// next N polys in <indices> represent open VD cells, thay are in order
	// and corresponding to vertices from GetFirstBoundaryVertex() -> Vertex::next list
	// every poly written to <indices> is terminated with ~0 value
	// if both <indices> and <closed_indices> are not null, 
	// number of closed VD cells indices is written to <closed_indices>
	// function returns number of indices filled (I) on success, otherwise 0
	virtual I GenVoronoiDiagramPolys(I* indices, size_t advance_bytes=0, I* closed_indices=0) const = 0;

	virtual void CheckTopology() const = 0;

	protected: virtual ~IDelaBella2() = 0; // please use Destroy()
};

template <typename T, typename I>
inline const typename IDelaBella2<T,I>::Simplex* IDelaBella2<T,I>::Simplex::StartIterator(IDelaBella2<T,I>::Iterator* it/*not_null*/, int around/*0,1,2*/) const
{
	it->current = this;
	it->around = around;
	return this;
}

template <typename T, typename I>
inline const typename IDelaBella2<T,I>::Simplex* IDelaBella2<T,I>::Vertex::StartIterator(IDelaBella2<T,I>::Iterator* it/*not_null*/) const
{
	it->current = sew;
	if (sew->v[0] == this)
		it->around = 0;
	else
	if (sew->v[1] == this)
		it->around = 1;
	else
		it->around = 2;
	return sew;
}


#endif
