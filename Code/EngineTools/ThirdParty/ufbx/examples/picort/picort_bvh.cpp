#include "picort.h"
#include <thread>
#include <algorithm>

struct BVHBucket
{
	Bounds bounds;
	size_t count = 0;

	inline void add(const Bounds &b) { bounds.add(b); count++; }
	inline void add(const BVHBucket &b) { bounds.add(b.bounds); count += b.count; }
	Real cost(Real parent_area) {
		return bounds.area() / parent_area * (Real)((count + 3) / 4);
	}
};

struct BVHSplit
{
	Real cost = Inf;
	int axis = -1;
	size_t bucket = 0;
	BVHBucket left, right;
};

Bounds merge_triangle_bounds(const Triangle *triangles, size_t count)
{
	Bounds bounds;
	for (size_t i = 0; i < count; i++) bounds.add(triangles[i].bounds());
	return bounds;
}

BVHBucket merge_buckets(const BVHBucket *buckets, size_t count)
{
	BVHBucket bucket;
	for (size_t i = 0; i < count; i++) bucket.add(buckets[i]);
	return bucket;
}

void bvh_recurse(BVH &bvh, Triangle *triangles, size_t count, const Bounds &bounds, int depth);

// Split a BVH node to two containing equal amount of triangles on the largest axis
void bvh_split_equal(BVH &bvh, Triangle *triangles, size_t count, const Bounds &bounds, int depth)
{
	int axis = largest_axis(bounds.diagonal());
	std::sort(triangles, triangles + count, [axis](const Triangle &a, const Triangle &b) {
		return a.bounds().center()[axis] < b.bounds().center()[axis]; });

	size_t num_left = count / 2, num_right = count - num_left;
	Triangle *right = triangles + num_left;
	bvh.child_bounds[0] = merge_triangle_bounds(triangles, num_left);
	bvh.child_bounds[1] = merge_triangle_bounds(right, num_right);

	bvh.child_nodes = std::make_unique<BVH[]>(2);
	bvh_recurse(bvh.child_nodes[0], triangles, num_left, bvh.child_bounds[0], depth + 1);
	bvh_recurse(bvh.child_nodes[1], right, num_right, bvh.child_bounds[1], depth + 1);
}

// Split a BVH node based on the SAH heuristic
// https://pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies#TheSurfaceAreaHeuristic
void bvh_split_bucketed(BVH &bvh, Triangle *triangles, size_t count, const Bounds &bounds, int depth)
{
	constexpr size_t num_buckets = 8;
	BVHBucket buckets[3][num_buckets];

	// Initialized buckets with triangles
	Real nb = (Real)num_buckets - 0.0001f;
	Vec3 to_bucket = Vec3{ nb, nb, nb } / (bounds.max - bounds.min);
	for (size_t i = 0; i < count; i++) {
		Triangle &tri = triangles[i];
		Bounds tri_bounds = tri.bounds();
		Vec3 t = (tri_bounds.center() - bounds.min) * to_bucket;
		t = min(max(t, Vec3{}), Vec3{ nb, nb, nb });
		buckets[0][(size_t)t.x].add(tri_bounds);
		buckets[1][(size_t)t.y].add(tri_bounds);
		buckets[2][(size_t)t.z].add(tri_bounds);
	}

	// Find the best split that minimizes the estimated SAH cost
	BVHSplit split;
	Real parent_area = bounds.area();
	for (int axis = 0; axis < 3; axis++) {
		for (size_t bucket = 1; bucket < num_buckets - 1; bucket++) {
			BVHBucket left = merge_buckets(buckets[axis], bucket);
			BVHBucket right = merge_buckets(buckets[axis] + bucket, num_buckets - bucket);
			if (left.count == 0 || right.count == 0) continue;

			Real cost = 0.5f + left.cost(parent_area) + right.cost(parent_area);
			if (cost < split.cost) {
				split = { cost, axis, bucket, left, right };
			}
		}
	}

	Real leaf_cost = (Real)((count + 3) / 4);

	// Partition the triangles to the left and right halves and recurse if we found
	// a split better than a leaf node, otherwise create a leaf or an equal split BVH.
	if (split.axis >= 0 && (split.cost < leaf_cost || count > 16)) {
		Triangle *first = triangles, *last = triangles + count;
		while (first != last) {
			Bounds tri_bounds = first->bounds();
			Vec3 t = (tri_bounds.center() - bounds.min) * to_bucket;
			t = min(max(t, Vec3{}), Vec3{ nb, nb, nb });
			if ((size_t)t[split.axis] < split.bucket) {
				++first;
			} else {
				std::swap(*first, *--last);
			}
		}

		size_t num_left = first - triangles, num_right = count - num_left;
		assert(num_left == split.left.count && num_right == split.right.count);
		bvh.child_bounds[0] = split.left.bounds;
		bvh.child_bounds[1] = split.right.bounds;
		bvh.child_nodes = std::make_unique<BVH[]>(2);

		// TODO: Thread limit
		if (depth < 4 && (num_left > count / 8 || num_right > count / 8)) {
			std::thread a { bvh_recurse, std::ref(bvh.child_nodes[0]), triangles, num_left, bvh.child_bounds[0], depth + 1 };
			std::thread b { bvh_recurse, std::ref(bvh.child_nodes[1]), first, num_right, bvh.child_bounds[1], depth + 1 };
			a.join();
			b.join();
		} else {
			bvh_recurse(bvh.child_nodes[0], triangles, num_left, bvh.child_bounds[0], depth + 1);
			bvh_recurse(bvh.child_nodes[1], first, num_right, bvh.child_bounds[1], depth + 1);
		}

	} else if (count > 16) {
		bvh_split_equal(bvh, triangles, count, bounds, depth);
	} else {
		bvh.triangles = std::vector<Triangle>{ triangles, triangles + count };
	}
}

// Choose a split method for the current BVH node
void bvh_recurse(BVH &bvh, Triangle *triangles, size_t count, const Bounds &bounds, int depth)
{
	if (count <= 4) {
		bvh.triangles = std::vector<Triangle>{ triangles, triangles + count };
	} else if (depth > 32) {
		bvh_split_equal(bvh, triangles, count, bounds, depth);
	} else {
		bvh_split_bucketed(bvh, triangles, count, bounds, depth);
	}
}

// Build a BVH structure from `count` `triangles`.
BVH build_bvh(Triangle *triangles, size_t count)
{
	BVH bvh;
	Bounds bounds = merge_triangle_bounds(triangles, count);
	bvh_recurse(bvh, triangles, count, bounds, 0);
	return bvh;
}

// Precompute some values to trace with for `ray`
RayTrace setup_trace(const Ray &ray, Real max_t)
{
	int kz = largest_axis(abs(ray.direction));
	int kx = (kz + 1) % 3, ky = (kz + 2) % 3;
	if (ray.direction[kz] < 0.0f) std::swap(kx, ky);

	RayTrace trace;
	trace.ray = ray;
	trace.rcp_direction = Vec3{1.0f, 1.0f, 1.0f} / ray.direction;
	trace.axes = { kx, ky, kz };
	trace.shear = Vec3{ray.direction[kx], ray.direction[ky], 1.0f} / ray.direction[kz];
	trace.max_t = max_t;
	return trace;
}

// Test if `trace` intersects `bounds`, returns front intersection t value
// https://pbr-book.org/3ed-2018/Shapes/Basic_Shape_Interface#RayndashBoundsIntersections
picort_forceinline Real intersect_bounds(const RayTrace &trace, const Bounds &bounds)
{
	Vec3 ts_min = (bounds.min - trace.ray.origin) * trace.rcp_direction;
	Vec3 ts_max = (bounds.max - trace.ray.origin) * trace.rcp_direction;
	Vec3 ts_near = min(ts_min, ts_max);
	Vec3 ts_far = max(ts_min, ts_max);
	Real t_near = max(max_component(ts_near), 0.0f);
	Real t_far = min_component(ts_far);
	return t_near <= t_far ? t_near : Inf;
}

// Check if `trace` intersects `triangle` in a watertight manner, updates `trace.hit`
// http://jcgt.org/published/0002/01/05/
// https://pbr-book.org/3ed-2018/Shapes/Triangle_Meshes#TriangleIntersection
void intersect_triangle(RayTrace &trace, const Triangle &triangle)
{
	int kx = trace.axes.x, ky = trace.axes.y, kz = trace.axes.z;
	Vec3 s = trace.shear;

	Vec3 a = triangle.v[0] - trace.ray.origin;
	Vec3 b = triangle.v[1] - trace.ray.origin;
	Vec3 c = triangle.v[2] - trace.ray.origin;

	Real ax = a[kx] - s.x*a[kz], ay = a[ky] - s.y*a[kz];
	Real bx = b[kx] - s.x*b[kz], by = b[ky] - s.y*b[kz];
	Real cx = c[kx] - s.x*c[kz], cy = c[ky] - s.y*c[kz];

	Real u = cx*by - cy*bx;
	Real v = ax*cy - ay*cx;
	Real w = bx*ay - by*ax;
	if (sizeof(Real) < 8 && (u == 0.0f || v == 0.0f || w == 0.0f)) {
		using D = double;
		u = (Real)((D)cx*(D)by - (D)cy*(D)bx);
		v = (Real)((D)ax*(D)cy - (D)ay*(D)cx);
		w = (Real)((D)bx*(D)ay - (D)by*(D)ax);
	}

	if ((u<0.0f || v<0.0f || w<0.0f) && (u>0.0f || v>0.0f || w>0.0f)) return;

	Real det = u + v + w;
	Real t = u*s.z*a[kz] + v*s.z*b[kz] + w*s.z*c[kz];

	if (det == 0.0f) return;
	if (det < 0.0f && (t >= 0.0f || t < trace.max_t * det)) return;
	if (det > 0.0f && (t <= 0.0f || t > trace.max_t * det)) return;

	Real rcp_det = 1.0f / det;
	trace.max_t = t * rcp_det;
	trace.hit = { t * rcp_det, u * rcp_det, v * rcp_det, triangle.index, trace.hit.steps };
}

struct BVHHit
{
	const BVH *bvh;
	Real t;
};

// Check if `trace` intersects anything within `bvh`, updates `trace.hit`
void intersect_bvh(RayTrace &trace, const BVH &root)
{
	BVHHit stack[64];
	uint32_t depth = 1;
	stack[0].bvh = &root;
	stack[0].t = 0.0f;

	while (depth > 0) {
		depth--;
		if (stack[depth].t >= trace.max_t) continue;
		const BVH *bvh = stack[depth].bvh;

		while (bvh && bvh->triangles.empty()) {
			trace.hit.steps++;
			const BVH *child0 = &bvh->child_nodes[0], *child1 = &bvh->child_nodes[1];
			Real t0 = intersect_bounds(trace, bvh->child_bounds[0]);
			Real t1 = intersect_bounds(trace, bvh->child_bounds[1]);
			bvh = NULL;
			if (t0 < t1) {
				if (t1 < trace.max_t) {
					stack[depth].bvh = child1;
					stack[depth].t = t1;
					depth++;
				}
				if (t0 < trace.max_t) {
					bvh = child0;
				}
			} else {
				if (t0 < trace.max_t) {
					stack[depth].bvh = child0;
					stack[depth].t = t0;
					depth++;
				}
				if (t1 < trace.max_t) {
					bvh = child1;
				}
			}
		}

		if (bvh && !bvh->triangles.empty()) {
			trace.hit.steps++;
			for (const Triangle &triangle : bvh->triangles) {
				intersect_triangle(trace, triangle);
			}
		}
	}
}
