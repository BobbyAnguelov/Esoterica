#include "external/sokol_app.h"
#include "external/sokol_gfx.h"
#include "external/sokol_time.h"
#include "external/sokol_glue.h"
#include "external/umath.h"
#include "../../ufbx.h"

#include "shaders/mesh.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define MAX_BONES 64
#define MAX_BLEND_SHAPES 64

um_vec2 ufbx_to_um_vec2(ufbx_vec2 v) { return um_v2((float)v.x, (float)v.y); }
um_vec3 ufbx_to_um_vec3(ufbx_vec3 v) { return um_v3((float)v.x, (float)v.y, (float)v.z); }
um_quat ufbx_to_um_quat(ufbx_quat v) { return um_quat_xyzw((float)v.x, (float)v.y, (float)v.z, (float)v.w); }
um_mat ufbx_to_um_mat(ufbx_matrix m) {
	return um_mat_rows(
		(float)m.m00, (float)m.m01, (float)m.m02, (float)m.m03,
		(float)m.m10, (float)m.m11, (float)m.m12, (float)m.m13,
		(float)m.m20, (float)m.m21, (float)m.m22, (float)m.m23,
		0, 0, 0, 1,
	);
}

typedef struct mesh_vertex {
	um_vec3 position;
	um_vec3 normal;
	um_vec2 uv;
	float f_vertex_index;
} mesh_vertex;

typedef struct skin_vertex {
	uint8_t bone_index[4];
	uint8_t bone_weight[4];
} skin_vertex;

static const sg_layout_desc mesh_vertex_layout = {
	.attrs = {
		{ .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },
		{ .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },
		{ .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT2 },
		{ .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT },
	},
};

static const sg_layout_desc skinned_mesh_vertex_layout = {
	.attrs = {
		{ .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },
		{ .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },
		{ .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT2 },
		{ .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT },
		{ .buffer_index = 1, .format = SG_VERTEXFORMAT_BYTE4 },
		{ .buffer_index = 1, .format = SG_VERTEXFORMAT_UBYTE4N },
	},
};

void print_error(const ufbx_error *error, const char *description)
{
	char buffer[1024];
	ufbx_format_error(buffer, sizeof(buffer), error);
	fprintf(stderr, "%s\n%s\n", description, buffer);
}

void *alloc_imp(size_t type_size, size_t count)
{
	void *ptr = malloc(type_size * count);
	if (!ptr) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	memset(ptr, 0, type_size * count);
	return ptr;
}

void *alloc_dup_imp(size_t type_size, size_t count, const void *data)
{
	void *ptr = malloc(type_size * count);
	if (!ptr) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	memcpy(ptr, data, type_size * count);
	return ptr;
}

#define alloc(m_type, m_count) (m_type*)alloc_imp(sizeof(m_type), (m_count))
#define alloc_dup(m_type, m_count, m_data) (m_type*)alloc_dup_imp(sizeof(m_type), (m_count), (m_data))

size_t min_sz(size_t a, size_t b) { return a < b ? a : b; }
size_t max_sz(size_t a, size_t b) { return b < a ? a : b; }
size_t clamp_sz(size_t a, size_t min_a, size_t max_a) { return min_sz(max_sz(a, min_a), max_a); }

typedef struct viewer_node_anim {
	float time_begin;
	float framerate;
	size_t num_frames;
	um_quat const_rot;
	um_vec3 const_pos;
	um_vec3 const_scale;
	um_quat *rot;
	um_vec3 *pos;
	um_vec3 *scale;
} viewer_node_anim;

typedef struct viewer_blend_channel_anim {
	float const_weight;
	float *weight;
} viewer_blend_channel_anim;

typedef struct viewer_anim {
	const char *name;
	float time_begin;
	float time_end;
	float framerate;
	size_t num_frames;

	viewer_node_anim *nodes;
	viewer_blend_channel_anim *blend_channels;
} viewer_anim;

typedef struct viewer_node {
	int32_t parent_index;

	um_mat geometry_to_node;
	um_mat node_to_parent;
	um_mat node_to_world;
	um_mat geometry_to_world;
	um_mat normal_to_world;
} viewer_node;

typedef struct viewer_blend_channel {
	float weight;
} viewer_blend_channel;

typedef struct viewer_mesh_part {
	sg_buffer vertex_buffer;
	sg_buffer index_buffer;
	sg_buffer skin_buffer; // Optional

	size_t num_indices;
	int32_t material_index;
} viewer_mesh_part;

typedef struct viewer_mesh {

	int32_t *instance_node_indices;
	size_t num_instances;

	viewer_mesh_part *parts;
	size_t num_parts;

	bool aabb_is_local;
	um_vec3 aabb_min;
	um_vec3 aabb_max;

	// Skinning (optional)
	bool skinned;
	size_t num_bones;
	int32_t bone_indices[MAX_BONES];
	um_mat bone_matrices[MAX_BONES];

	// Blend shapes (optional)
	size_t num_blend_shapes;
	sg_image blend_shape_image;
	int32_t blend_channel_indices[MAX_BLEND_SHAPES];

} viewer_mesh;

typedef struct viewer_scene {

	viewer_node *nodes;
	size_t num_nodes;

	viewer_mesh *meshes;
	size_t num_meshes;

	viewer_blend_channel *blend_channels;
	size_t num_blend_channels;

	viewer_anim *animations;
	size_t num_animations;

	um_vec3 aabb_min;
	um_vec3 aabb_max;

} viewer_scene;

typedef struct viewer {

	viewer_scene scene;
	float anim_time;

	sg_shader shader_mesh_lit_static;
	sg_shader shader_mesh_lit_skinned;
	sg_pipeline pipe_mesh_lit_static;
	sg_pipeline pipe_mesh_lit_skinned;
	sg_image empty_blend_shape_image;

	um_mat world_to_view;
	um_mat view_to_clip;
	um_mat world_to_clip;

	float camera_yaw;
	float camera_pitch;
	float camera_distance;
	uint32_t mouse_buttons;
} viewer;

void read_node(viewer_node *vnode, ufbx_node *node)
{
	vnode->parent_index = node->parent ? node->parent->typed_id : -1;
	vnode->node_to_parent = ufbx_to_um_mat(node->node_to_parent);
	vnode->node_to_world = ufbx_to_um_mat(node->node_to_world);
	vnode->geometry_to_node = ufbx_to_um_mat(node->geometry_to_node);
	vnode->geometry_to_world = ufbx_to_um_mat(node->geometry_to_world);
	vnode->normal_to_world = ufbx_to_um_mat(ufbx_matrix_for_normals(&node->geometry_to_world));
}

sg_image pack_blend_channels_to_image(ufbx_mesh *mesh, ufbx_blend_channel **channels, size_t num_channels)
{
	// We pack the blend shape data into a 1024xNxM texture array where each texel
	// contains the vertex `Y*1024 + X` for blend shape `Z`.
	uint32_t tex_width = 1024;
	uint32_t tex_height_min = ((uint32_t)mesh->num_vertices + tex_width - 1) / tex_width;
	uint32_t tex_slices = (uint32_t)num_channels;

	// Let's make the texture size a power of two just to be sure...
	uint32_t tex_height = 1;
	while (tex_height < tex_height_min) {
		tex_height *= 2;
	}

	// NOTE: A proper implementation would probably compress the shape offsets to FP16
	// or some other quantization to save space, we use full FP32 here for simplicity.
	size_t tex_texels = tex_width * tex_height * tex_slices;
	um_vec4 *tex_data = alloc(um_vec4, tex_texels);

	// Copy the vertex offsets from each blend shape
	for (uint32_t ci = 0; ci < num_channels; ci++) {
		ufbx_blend_channel *chan = channels[ci];
		um_vec4 *slice_data = tex_data + tex_width * tex_height * ci;
	
		// Let's use the last blend shape if there's multiple blend phases as we don't
		// support it. Fortunately this feature is quite rarely used in practice.
		ufbx_blend_shape *shape = chan->keyframes.data[chan->keyframes.count - 1].shape;

		for (size_t oi = 0; oi < shape->num_offsets; oi++) {
			uint32_t ix = (uint32_t)shape->offset_vertices.data[oi];
			if (ix < mesh->num_vertices) {
				// We don't need to do any indexing to X/Y here as the memory layout of
				// `slice_data` pixels is the same as the linear buffer would be.
				slice_data[ix].xyz = ufbx_to_um_vec3(shape->position_offsets.data[oi]);
			}
		}
	}

	// Upload the combined blend offset image to the GPU
	sg_image image = sg_make_image(&(sg_image_desc){
		.type = SG_IMAGETYPE_ARRAY,
		.width = (int)tex_width,
		.height = (int)tex_height,
		.num_slices = tex_slices,
		.pixel_format = SG_PIXELFORMAT_RGBA32F,
		.data.subimage[0][0] = { tex_data, tex_texels * sizeof(um_vec4) },
	});

	free(tex_data);

	return image;
}

void read_mesh(viewer_mesh *vmesh, ufbx_mesh *mesh)
{
	// Count the number of needed parts and temporary buffers
	size_t max_parts = 0;
	size_t max_triangles = 0;

	// We need to render each material of the mesh in a separate part, so let's
	// count the number of parts and maximum number of triangles needed.
	for (size_t pi = 0; pi < mesh->material_parts.count; pi++) {
		ufbx_mesh_part *part = &mesh->material_parts.data[pi];
		if (part->num_triangles == 0) continue;
		max_parts += 1;
		max_triangles = max_sz(max_triangles, part->num_triangles);
	}

	// Temporary buffers
	size_t num_tri_indices = mesh->max_face_triangles * 3;
	uint32_t *tri_indices = alloc(uint32_t, num_tri_indices);
	mesh_vertex *vertices = alloc(mesh_vertex, max_triangles * 3);
	skin_vertex *skin_vertices = alloc(skin_vertex, max_triangles * 3);
	skin_vertex *mesh_skin_vertices = alloc(skin_vertex, mesh->num_vertices);
	uint32_t *indices = alloc(uint32_t, max_triangles * 3);

	// Result buffers
	viewer_mesh_part *parts = alloc(viewer_mesh_part, max_parts);
	size_t num_parts = 0;

	// In FBX files a single mesh can be instanced by multiple nodes. ufbx handles the connection
	// in two ways: (1) `ufbx_node.mesh/light/camera/etc` contains pointer to the data "attribute"
	// that node uses and (2) each element that can be connected to a node contains a list of
	// `ufbx_node*` instances eg. `ufbx_mesh.instances`.
	vmesh->num_instances = mesh->instances.count;
	vmesh->instance_node_indices = alloc(int32_t, mesh->instances.count);
	for (size_t i = 0; i < mesh->instances.count; i++) {
		vmesh->instance_node_indices[i] = (int32_t)mesh->instances.data[i]->typed_id;
	}

	// Create the vertex buffers
	size_t num_blend_shapes = 0;
	ufbx_blend_channel *blend_channels[MAX_BLEND_SHAPES];
	size_t num_bones = 0;
	ufbx_skin_deformer *skin = NULL;

	if (mesh->skin_deformers.count > 0) {
		vmesh->skinned = true;

		// Having multiple skin deformers attached at once is exceedingly rare so we can just
		// pick the first one without having to worry too much about it.
		skin = mesh->skin_deformers.data[0];

		// NOTE: A proper implementation would split meshes with too many bones to chunks but
		// for simplicity we're going to just pick the first `MAX_BONES` ones.
		for (size_t ci = 0; ci < skin->clusters.count; ci++) {
			ufbx_skin_cluster *cluster = skin->clusters.data[ci];
			if (num_bones < MAX_BONES) {
				vmesh->bone_indices[num_bones] = (int32_t)cluster->bone_node->typed_id;
				vmesh->bone_matrices[num_bones] = ufbx_to_um_mat(cluster->geometry_to_bone);
				num_bones++;
			}
		}
		vmesh->num_bones = num_bones;

		// Pre-calculate the skinned vertex bones/weights for each vertex as they will probably
		// be shared by multiple indices.
		for (size_t vi = 0; vi < mesh->num_vertices; vi++) {
			size_t num_weights = 0;
			float total_weight = 0.0f;
			float weights[4] = { 0.0f };
			uint8_t clusters[4] = { 0 };

			// `ufbx_skin_vertex` contains the offset and number of weights that deform the vertex
			// in a descending weight order so we can pick the first N weights to use and get a
			// reasonable approximation of the skinning.
			ufbx_skin_vertex vertex_weights = skin->vertices.data[vi];
			for (size_t wi = 0; wi < vertex_weights.num_weights; wi++) {
				if (num_weights >= 4) break;
				ufbx_skin_weight weight = skin->weights.data[vertex_weights.weight_begin + wi];

				// Since we only support a fixed amount of bones up to `MAX_BONES` and we take the
				// first N ones we need to ignore weights with too high `cluster_index`.
				if (weight.cluster_index < MAX_BONES) {
					total_weight += (float)weight.weight;
					clusters[num_weights] = (uint8_t)weight.cluster_index;
					weights[num_weights] = (float)weight.weight;
					num_weights++;
				}
			}

			// Normalize and quantize the weights to 8 bits. We need to be a bit careful to make
			// sure the _quantized_ sum is normalized ie. all 8-bit values sum to 255.
			if (total_weight > 0.0f) {
				skin_vertex *skin_vert = &mesh_skin_vertices[vi];
				uint32_t quantized_sum = 0;
				for (size_t i = 0; i < 4; i++) {
					uint8_t quantized_weight = (uint8_t)((float)weights[i] / total_weight * 255.0f);
					quantized_sum += quantized_weight;
					skin_vert->bone_index[i] = clusters[i];
					skin_vert->bone_weight[i] = quantized_weight;
				}
				skin_vert->bone_weight[0] += 255 - quantized_sum;
			}
		}
	}

	// Fetch blend channels from all attached blend deformers.
	for (size_t di = 0; di < mesh->blend_deformers.count; di++) {
		ufbx_blend_deformer *deformer = mesh->blend_deformers.data[di];
		for (size_t ci = 0; ci < deformer->channels.count; ci++) {
			ufbx_blend_channel *chan = deformer->channels.data[ci];
			if (chan->keyframes.count == 0) continue;
			if (num_blend_shapes < MAX_BLEND_SHAPES) {
				blend_channels[num_blend_shapes] = chan;
				vmesh->blend_channel_indices[num_blend_shapes] = (int32_t)chan->typed_id;
				num_blend_shapes++;
			}
		}
	}
	if (num_blend_shapes > 0) {
		vmesh->blend_shape_image = pack_blend_channels_to_image(mesh, blend_channels, num_blend_shapes);
		vmesh->num_blend_shapes = num_blend_shapes;
	}

	// Our shader supports only a single material per draw call so we need to split the mesh
	// into parts by material. `ufbx_mesh_part` contains a handy compact list of faces
	// that use the material which we use here.
	for (size_t pi = 0; pi < mesh->material_parts.count; pi++) {
		ufbx_mesh_part *mesh_part = &mesh->material_parts.data[pi];
		if (mesh_part->num_triangles == 0) continue;

		viewer_mesh_part *part = &parts[num_parts++];
		size_t num_indices = 0;

		// First fetch all vertices into a flat non-indexed buffer, we also need to
		// triangulate the faces
		for (size_t fi = 0; fi < mesh_part->num_faces; fi++) {
			ufbx_face face = mesh->faces.data[mesh_part->face_indices.data[fi]];
			size_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);

			ufbx_vec2 default_uv = { 0 };

			// Iterate through every vertex of every triangle in the triangulated result
			for (size_t vi = 0; vi < num_tris * 3; vi++) {
				uint32_t ix = tri_indices[vi];
				mesh_vertex *vert = &vertices[num_indices];

				ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
				ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, ix);
				ufbx_vec2 uv = mesh->vertex_uv.exists ? ufbx_get_vertex_vec2(&mesh->vertex_uv, ix) : default_uv;

				vert->position = ufbx_to_um_vec3(pos);
				vert->normal = um_normalize3(ufbx_to_um_vec3(normal));
				vert->uv = ufbx_to_um_vec2(uv);
				vert->f_vertex_index = (float)mesh->vertex_indices.data[ix];

				// The skinning vertex stream is pre-calculated above so we just need to
				// copy the right one by the vertex index.
				if (skin) {
					skin_vertices[num_indices] = mesh_skin_vertices[mesh->vertex_indices.data[ix]];
				}

				num_indices++;
			}
		}

		ufbx_vertex_stream streams[2];
		size_t num_streams = 1;

		streams[0].data = vertices;
		streams[0].vertex_count = num_indices;
		streams[0].vertex_size = sizeof(mesh_vertex);

		if (skin) {
			streams[1].data = skin_vertices;
			streams[1].vertex_count = num_indices;
			streams[1].vertex_size = sizeof(skin_vertex);
			num_streams = 2;
		}

		// Optimize the flat vertex buffer into an indexed one. `ufbx_generate_indices()`
		// compacts the vertex buffer and returns the number of used vertices.
		ufbx_error error;
		size_t num_vertices = ufbx_generate_indices(streams, num_streams, indices, num_indices, NULL, &error);
		if (error.type != UFBX_ERROR_NONE) {
			print_error(&error, "Failed to generate index buffer");
			exit(1);
		}

		part->num_indices = num_indices;
		if (mesh_part->index < mesh->materials.count) {
			ufbx_material *material =  mesh->materials.data[mesh_part->index];
			part->material_index = (int32_t)material->typed_id;
		} else {
			part->material_index = -1;
		}

		// Create the GPU buffers from the temporary `vertices` and `indices` arrays
		part->index_buffer = sg_make_buffer(&(sg_buffer_desc){
			.size = num_indices * sizeof(uint32_t),
			.type = SG_BUFFERTYPE_INDEXBUFFER,
			.data = { indices, num_indices * sizeof(uint32_t) },
		});
		part->vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
			.size = num_vertices * sizeof(mesh_vertex),
			.type = SG_BUFFERTYPE_VERTEXBUFFER,
			.data = { vertices, num_vertices * sizeof(mesh_vertex) },
		});

		if (vmesh->skinned) {
			part->skin_buffer = sg_make_buffer(&(sg_buffer_desc){
				.size = num_vertices * sizeof(skin_vertex),
				.type = SG_BUFFERTYPE_VERTEXBUFFER,
				.data = { skin_vertices, num_vertices * sizeof(skin_vertex) },
			});
		}
	}

	// Free the temporary buffers
	free(tri_indices);
	free(vertices);
	free(skin_vertices);
	free(mesh_skin_vertices);
	free(indices);

	// Compute bounds from the vertices
	vmesh->aabb_is_local = mesh->skinned_is_local;
	vmesh->aabb_min = um_dup3(+INFINITY);
	vmesh->aabb_max = um_dup3(-INFINITY);
	for (size_t i = 0; i < mesh->num_vertices; i++) {
		um_vec3 pos = ufbx_to_um_vec3(mesh->skinned_position.values.data[i]);
		vmesh->aabb_min = um_min3(vmesh->aabb_min, pos);
		vmesh->aabb_max = um_max3(vmesh->aabb_max, pos);
	}

	vmesh->parts = parts;
	vmesh->num_parts = num_parts;
}

void read_blend_channel(viewer_blend_channel *vchan, ufbx_blend_channel *chan)
{
	vchan->weight = (float)chan->weight;
}

void read_node_anim(viewer_anim *va, viewer_node_anim *vna, ufbx_anim_stack *stack, ufbx_node *node)
{
	vna->rot = alloc(um_quat, va->num_frames);
	vna->pos = alloc(um_vec3, va->num_frames);
	vna->scale = alloc(um_vec3, va->num_frames);

	bool const_rot = true, const_pos = true, const_scale = true;

	// Sample the node's transform evenly for the whole animation stack duration
	for (size_t i = 0; i < va->num_frames; i++) {
		double time = stack->time_begin + (double)i / va->framerate;

		ufbx_transform transform = ufbx_evaluate_transform(stack->anim, node, time);
		vna->rot[i] = ufbx_to_um_quat(transform.rotation);
		vna->pos[i] = ufbx_to_um_vec3(transform.translation);
		vna->scale[i] = ufbx_to_um_vec3(transform.scale);

		if (i > 0) {
			// Negated quaternions are equivalent, but interpolating between ones of different
			// polarity takes a the longer path, so flip the quaternion if necessary.
			if (um_quat_dot(vna->rot[i], vna->rot[i - 1]) < 0.0f) {
				vna->rot[i] = um_quat_neg(vna->rot[i]);
			}

			// Keep track of which channels are constant for the whole animation as an optimization
			if (!um_quat_equal(vna->rot[i - 1], vna->rot[i])) const_rot = false;
			if (!um_equal3(vna->pos[i - 1], vna->pos[i])) const_pos = false;
			if (!um_equal3(vna->scale[i - 1], vna->scale[i])) const_scale = false;
		}
	}

	if (const_rot) { vna->const_rot = vna->rot[0]; free(vna->rot); vna->rot = NULL; }
	if (const_pos) { vna->const_pos = vna->pos[0]; free(vna->pos); vna->pos = NULL; }
	if (const_scale) { vna->const_scale = vna->scale[0]; free(vna->scale); vna->scale = NULL; }
}

void read_blend_channel_anim(viewer_anim *va, viewer_blend_channel_anim *vbca, ufbx_anim_stack *stack, ufbx_blend_channel *chan)
{
	vbca->weight = alloc(float, va->num_frames);

	bool const_weight = true;

	// Sample the blend weight evenly for the whole animation stack duration
	for (size_t i = 0; i < va->num_frames; i++) {
		double time = stack->time_begin + (double)i / va->framerate;

		ufbx_real weight = ufbx_evaluate_blend_weight(stack->anim, chan, time);
		vbca->weight[i] = (float)weight;

		// Keep track of which channels are constant for the whole animation as an optimization
		if (i > 0) {
			if (vbca->weight[i - 1] != vbca->weight[i]) const_weight = false;
		}
	}

	if (const_weight) { vbca->const_weight = vbca->weight[0]; free(vbca->weight); vbca->weight = NULL; }
}

void read_anim_stack(viewer_anim *va, ufbx_anim_stack *stack, ufbx_scene *scene)
{
	const float target_framerate = 30.0f;
	const size_t max_frames = 4096;

	// Sample the animation evenly at `target_framerate` if possible while limiting the maximum
	// number of frames to `max_frames` by potentially dropping FPS.
	float duration = (float)stack->time_end - (float)stack->time_begin;
	size_t num_frames = clamp_sz((size_t)(duration * target_framerate), 2, max_frames);
	float framerate = (float)(num_frames - 1) / duration;

	va->name = alloc_dup(char, stack->name.length + 1, stack->name.data);
	va->time_begin = (float)stack->time_begin;
	va->time_end = (float)stack->time_end;
	va->framerate = framerate;
	va->num_frames = num_frames;

	// Sample the animations of all nodes and blend channels in the stack
	va->nodes = alloc(viewer_node_anim, scene->nodes.count);
	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node *node = scene->nodes.data[i];
		read_node_anim(va, &va->nodes[i], stack, node);
	}

	va->blend_channels = alloc(viewer_blend_channel_anim, scene->blend_channels.count);
	for (size_t i = 0; i < scene->blend_channels.count; i++) {
		ufbx_blend_channel *chan = scene->blend_channels.data[i];
		read_blend_channel_anim(va, &va->blend_channels[i], stack, chan);
	}
}

void read_scene(viewer_scene *vs, ufbx_scene *scene)
{
	vs->num_nodes = scene->nodes.count;
	vs->nodes = alloc(viewer_node, vs->num_nodes);
	for (size_t i = 0; i < vs->num_nodes; i++) {
		read_node(&vs->nodes[i], scene->nodes.data[i]);
	}

	vs->num_meshes = scene->meshes.count;
	vs->meshes = alloc(viewer_mesh, vs->num_meshes);
	for (size_t i = 0; i < vs->num_meshes; i++) {
		read_mesh(&vs->meshes[i], scene->meshes.data[i]);
	}

	vs->num_blend_channels = scene->blend_channels.count;
	vs->blend_channels = alloc(viewer_blend_channel, vs->num_blend_channels);
	for (size_t i = 0; i < vs->num_blend_channels; i++) {
		read_blend_channel(&vs->blend_channels[i], scene->blend_channels.data[i]);
	}

	vs->num_animations = scene->anim_stacks.count;
	vs->animations = alloc(viewer_anim, vs->num_animations);
	for (size_t i = 0; i < vs->num_animations; i++) {
		read_anim_stack(&vs->animations[i], scene->anim_stacks.data[i], scene);
	}
}

void update_animation(viewer_scene *vs, viewer_anim *va, float time)
{
	float frame_time = (time - va->time_begin) * va->framerate;
	size_t f0 = min_sz((size_t)frame_time + 0, va->num_frames - 1);
	size_t f1 = min_sz((size_t)frame_time + 1, va->num_frames - 1);
	float t = um_min(frame_time - (float)f0, 1.0f);

	for (size_t i = 0; i < vs->num_nodes; i++) {
		viewer_node *vn = &vs->nodes[i];
		viewer_node_anim *vna = &va->nodes[i];

		um_quat rot = vna->rot ? um_quat_lerp(vna->rot[f0], vna->rot[f1], t) : vna->const_rot;
		um_vec3 pos = vna->pos ? um_lerp3(vna->pos[f0], vna->pos[f1], t) : vna->const_pos;
		um_vec3 scale = vna->scale ? um_lerp3(vna->scale[f0], vna->scale[f1], t) : vna->const_scale;

		vn->node_to_parent = um_mat_trs(pos, rot, scale);
	}

	for (size_t i = 0; i < vs->num_blend_channels; i++) {
		viewer_blend_channel *vbc = &vs->blend_channels[i];
		viewer_blend_channel_anim *vbca = &va->blend_channels[i];

		vbc->weight = vbca->weight ? um_lerp(vbca->weight[f0], vbca->weight[f1], t) : vbca->const_weight;
	}
}

void update_hierarchy(viewer_scene *vs)
{
	for (size_t i = 0; i < vs->num_nodes; i++) {
		viewer_node *vn = &vs->nodes[i];

		// ufbx stores nodes in order where parent nodes always precede child nodes so we can
		// evaluate the transform hierarchy with a flat loop.
		if (vn->parent_index >= 0) {
			vn->node_to_world = um_mat_mul(vs->nodes[vn->parent_index].node_to_world, vn->node_to_parent);
		} else {
			vn->node_to_world = vn->node_to_parent;
		}
		vn->geometry_to_world = um_mat_mul(vn->node_to_world, vn->geometry_to_node);
		vn->normal_to_world = um_mat_transpose(um_mat_inverse(vn->geometry_to_world));
	}
}

void init_pipelines(viewer *view)
{
	sg_backend backend = sg_query_backend();

	view->shader_mesh_lit_static = sg_make_shader(static_lit_shader_desc(backend));
	view->pipe_mesh_lit_static = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = view->shader_mesh_lit_static,
		.layout = mesh_vertex_layout,
		.index_type = SG_INDEXTYPE_UINT32,
		.face_winding = SG_FACEWINDING_CCW,
		.cull_mode = SG_CULLMODE_BACK,
		.depth = {
			.compare = SG_COMPAREFUNC_LESS_EQUAL,
			.write_enabled = true,
		},
	});

	view->shader_mesh_lit_skinned = sg_make_shader(skinned_lit_shader_desc(backend));
	view->pipe_mesh_lit_skinned = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = view->shader_mesh_lit_skinned,
		.layout = skinned_mesh_vertex_layout,
		.index_type = SG_INDEXTYPE_UINT32,
		.face_winding = SG_FACEWINDING_CCW,
		.cull_mode = SG_CULLMODE_BACK,
		.depth = {
			.compare = SG_COMPAREFUNC_LESS_EQUAL,
			.write_enabled = true,
		},
	});

	um_vec4 empty_blend_shape_data = { 0 };
	view->empty_blend_shape_image = sg_make_image(&(sg_image_desc){
		.type = SG_IMAGETYPE_ARRAY,
		.width = 1,
		.height = 1,
		.num_slices = 1,
		.pixel_format = SG_PIXELFORMAT_RGBA32F,
		.data.subimage[0][0] = SG_RANGE(empty_blend_shape_data),
	});
}

void load_scene(viewer_scene *vs, const char *filename)
{
	ufbx_load_opts opts = {
		.load_external_files = true,
		.ignore_missing_external_files = true,
		.generate_missing_normals = true,

		// NOTE: We use this _only_ for computing the bounds of the scene!
		// The viewer contains a proper implementation of skinning as well.
		// You probably don't need this.
		.evaluate_skinning = true,

		.target_axes = {
			.right = UFBX_COORDINATE_AXIS_POSITIVE_X,
			.up = UFBX_COORDINATE_AXIS_POSITIVE_Y,
			.front = UFBX_COORDINATE_AXIS_POSITIVE_Z,
		},
		.target_unit_meters = 1.0f,
	};
	ufbx_error error;
	ufbx_scene *scene = ufbx_load_file(filename, &opts, &error);
	if (!scene) {
		print_error(&error, "Failed to load scene");
		exit(1);
	}

	read_scene(vs, scene);

	// Compute the world-space bounding box
	vs->aabb_min = um_dup3(+INFINITY);
	vs->aabb_max = um_dup3(-INFINITY);
	for (size_t mesh_ix = 0; mesh_ix < vs->num_meshes; mesh_ix++) {
		viewer_mesh *mesh = &vs->meshes[mesh_ix];
		um_vec3 aabb_origin = um_mul3(um_add3(mesh->aabb_max, mesh->aabb_min), 0.5f);
		um_vec3 aabb_extent = um_mul3(um_sub3(mesh->aabb_max, mesh->aabb_min), 0.5f);
		if (mesh->aabb_is_local) {
			for (size_t inst_ix = 0; inst_ix < mesh->num_instances; inst_ix++) {
				viewer_node *node = &vs->nodes[mesh->instance_node_indices[inst_ix]];
				um_vec3 world_origin = um_transform_point(&node->geometry_to_world, aabb_origin);
				um_vec3 world_extent = um_transform_extent(&node->geometry_to_world, aabb_extent);
				vs->aabb_min = um_min3(vs->aabb_min, um_sub3(world_origin, world_extent));
				vs->aabb_max = um_max3(vs->aabb_max, um_add3(world_origin, world_extent));
			}
		} else {
			vs->aabb_min = um_min3(vs->aabb_min, mesh->aabb_min);
			vs->aabb_max = um_max3(vs->aabb_max, mesh->aabb_max);
		}
	}

	ufbx_free_scene(scene);
}

bool backend_uses_d3d_perspective(sg_backend backend)
{
	switch (backend) {
	case SG_BACKEND_GLCORE33: return false;
    case SG_BACKEND_GLES2: return false;
    case SG_BACKEND_GLES3: return false;
    case SG_BACKEND_D3D11: return true;
    case SG_BACKEND_METAL_IOS: return true;
    case SG_BACKEND_METAL_MACOS: return true;
    case SG_BACKEND_METAL_SIMULATOR: return true;
    case SG_BACKEND_WGPU: return true;
    case SG_BACKEND_DUMMY: return false;
	default: assert(0 && "Unhandled backend"); return false;
	}
}

void update_camera(viewer *view)
{
	viewer_scene *vs = &view->scene;

	um_vec3 aabb_origin = um_mul3(um_add3(vs->aabb_max, vs->aabb_min), 0.5f);
	um_vec3 aabb_extent = um_mul3(um_sub3(vs->aabb_max, vs->aabb_min), 0.5f);
	float distance = 2.5f * powf(2.0f, view->camera_distance) * um_max(um_max(aabb_extent.x, aabb_extent.y), aabb_extent.z);

	um_quat camera_rot = um_quat_mul(
		um_quat_axis_angle(um_v3(0,1,0), view->camera_yaw * UM_DEG_TO_RAD),
		um_quat_axis_angle(um_v3(1,0,0), view->camera_pitch * UM_DEG_TO_RAD));

	um_vec3 camera_target = aabb_origin;
	um_vec3 camera_direction = um_quat_rotate(camera_rot, um_v3(0,0,1));
	um_vec3 camera_pos = um_add3(camera_target, um_mul3(camera_direction, distance));

	view->world_to_view = um_mat_look_at(camera_pos, camera_target, um_v3(0,1,0));

	float fov = 50.0f * UM_DEG_TO_RAD;
	float aspect = (float)sapp_width() / (float)sapp_height();
	float near_plane = um_min(distance * 0.001f, 0.1f);
	float far_plane = um_max(distance * 2.0f, 100.0f);

	if (backend_uses_d3d_perspective(sg_query_backend())) {
		view->view_to_clip = um_mat_perspective_d3d(fov, aspect, near_plane, far_plane);
	} else {
		view->view_to_clip = um_mat_perspective_gl(fov, aspect, near_plane, far_plane);
	}
	view->world_to_clip = um_mat_mul(view->view_to_clip, view->world_to_view);
}

void draw_mesh(viewer *view, viewer_node *node, viewer_mesh *mesh)
{
	sg_image blend_shapes = mesh->num_blend_shapes > 0 ? mesh->blend_shape_image : view->empty_blend_shape_image;

	if (mesh->skinned) {
		sg_apply_pipeline(view->pipe_mesh_lit_skinned);

		skin_vertex_ubo_t skin_ubo = { 0 };
		for (size_t i = 0; i < mesh->num_bones; i++) {
			viewer_node *bone = &view->scene.nodes[mesh->bone_indices[i]];
			skin_ubo.bones[i] = um_mat_mul(bone->node_to_world, mesh->bone_matrices[i]);
		}
		sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_skin_vertex_ubo, SG_RANGE_REF(skin_ubo));

	} else {
		sg_apply_pipeline(view->pipe_mesh_lit_static);

	}

	mesh_vertex_ubo_t mesh_ubo = {
		.geometry_to_world = node->geometry_to_world,
		.normal_to_world = node->normal_to_world,
		.world_to_clip = view->world_to_clip,
		.f_num_blend_shapes = (float)mesh->num_blend_shapes,
	};

	// sokol-shdc only supports vec4 arrays so reinterpret this `um_vec4` array as `float`
	float *blend_weights = (float*)mesh_ubo.blend_weights;
	for (size_t i = 0; i < mesh->num_blend_shapes; i++) {
		blend_weights[i] = view->scene.blend_channels[mesh->blend_channel_indices[i]].weight;
	}

	sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_mesh_vertex_ubo, SG_RANGE_REF(mesh_ubo));

	for (size_t pi = 0; pi < mesh->num_parts; pi++) {
		viewer_mesh_part *part = &mesh->parts[pi];

		sg_bindings binds = {
			.vertex_buffers[0] = part->vertex_buffer,
			.vertex_buffers[1] = part->skin_buffer,
			.index_buffer = part->index_buffer,
			.vs_images[SLOT_blend_shapes] = blend_shapes,
		};
		sg_apply_bindings(&binds);

		sg_draw(0, (int)part->num_indices, 1);
	}
}

void draw_scene(viewer *view)
{
	for (size_t mi = 0; mi < view->scene.num_meshes; mi++) {
		viewer_mesh *mesh = &view->scene.meshes[mi];
		for (size_t ni = 0; ni < mesh->num_instances; ni++) {
			viewer_node *node = &view->scene.nodes[mesh->instance_node_indices[ni]];
			draw_mesh(view, node, mesh);
		}
	}
}

viewer g_viewer;
const char *g_filename;

void init(void)
{
    sg_setup(&(sg_desc){
		.context = sapp_sgcontext(),
		.buffer_pool_size = 4096,
		.image_pool_size = 4096,
    });

	stm_setup();

	init_pipelines(&g_viewer);
	load_scene(&g_viewer.scene, g_filename);
}

void onevent(const sapp_event *e)
{
	viewer *view = &g_viewer;

	switch (e->type) {
	case SAPP_EVENTTYPE_MOUSE_DOWN:
		view->mouse_buttons |= 1u << (uint32_t)e->mouse_button;
		break;
	case SAPP_EVENTTYPE_MOUSE_UP:
		view->mouse_buttons &= ~(1u << (uint32_t)e->mouse_button);
		break;
	case SAPP_EVENTTYPE_UNFOCUSED:
		view->mouse_buttons = 0;
		break;
	case SAPP_EVENTTYPE_MOUSE_MOVE:
		if (view->mouse_buttons & 1) {
			float scale = um_min((float)sapp_width(), (float)sapp_height());
			view->camera_yaw -= e->mouse_dx / scale * 180.0f;
			view->camera_pitch -= e->mouse_dy / scale * 180.0f;
			view->camera_pitch = um_clamp(view->camera_pitch, -89.0f, 89.0f);
		}
		break;
	case SAPP_EVENTTYPE_MOUSE_SCROLL:
		view->camera_distance += e->scroll_y * -0.02f;
		view->camera_distance = um_clamp(view->camera_distance, -5.0f, 5.0f);
		break;
	default:
		break;
	}
}

void frame(void)
{
	static uint64_t last_time;
	float dt = (float)stm_sec(stm_laptime(&last_time));
	dt = um_min(dt, 0.1f);

	viewer_anim *anim = g_viewer.scene.num_animations > 0 ? &g_viewer.scene.animations[0] : NULL;

	if (anim) {
		g_viewer.anim_time += dt;
		if (g_viewer.anim_time >= anim->time_end) {
			g_viewer.anim_time -= anim->time_end - anim->time_begin;
		}
		update_animation(&g_viewer.scene, anim, g_viewer.anim_time);
	}

	update_camera(&g_viewer);
	update_hierarchy(&g_viewer.scene);

	sg_pass_action action = {
		.colors[0] = {
			.action = SG_ACTION_CLEAR,
			.value = { 0.1f, 0.1f, 0.2f },
		},
	};
    sg_begin_default_pass(&action, sapp_width(), sapp_height());

	draw_scene(&g_viewer);

    sg_end_pass();
    sg_commit();
}

void cleanup(void)
{
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {

	if (argc <= 1) {
		fprintf(stderr, "Usage: viewer file.fbx\n");
		exit(1);
	}

	g_filename = argv[1];

    return (sapp_desc){
        .init_cb = &init,
        .event_cb = &onevent,
        .frame_cb = &frame,
        .cleanup_cb = &cleanup,
        .width = 800,
        .height = 600,
		.sample_count = 4,
        .window_title = "ufbx viewer",
    };
}
