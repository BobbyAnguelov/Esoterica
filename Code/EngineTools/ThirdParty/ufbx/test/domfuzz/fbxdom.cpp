#include "fbxdom.h"
#include <assert.h>
#include <string.h>

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
	#define FBXDOM_NATIVE_WRITE 1
#else
	#define FBXDOM_NATIVE_WRITE 0
#endif

namespace fbxdom {

struct binary_reader
{
	const char *ptr;
	const char *start;
	const char *end;

	binary_reader(const void *data, size_t size)
		: ptr((const char*)data), start((const char*)data)
		, end((const char*)data + size) { }

	size_t offset() const { return ptr - start; }

	void skip(size_t size) {
		assert((size_t)(end - ptr) >= size);
		ptr += size;
	}

	template <typename T>
	T peek() {
		assert((size_t)(end - ptr) >= sizeof(T));
		uint64_t value = 0;
		for (size_t i = 0; i < sizeof(T); i++) {
			value |= (uint64_t)(uint8_t)ptr[i] << i * 8;
		}
		T result;
		memcpy(&result, &value, sizeof(T));
		return result;
	}

	template <typename T>
	T read() {
		T result = peek<T>();
		ptr += sizeof(T);
		return result;
	}

	std::vector<char> read_vector(size_t size)
	{
		std::vector<char> result;
		assert((size_t)(end - ptr) >= size);
		result.insert(result.begin(), ptr, ptr + size);
		ptr += size;
		return result;
	}
};

struct binary_writer
{
	char *ptr;
	char *start;
	char *end;

	binary_writer(const void *data, size_t size)
		: ptr((char*)data), start((char*)data)
		, end((char*)data + size) { }


	size_t offset() { return ptr - start; }

	void skip(size_t size)
	{
		assert((size_t)(end - ptr) >= size);
		for (size_t i = 0; i < size; i++) {
			*ptr++ = 0;
		}
	}

	template <typename T, typename U>
	void write(U value, size_t offset) {
#if FBXDOM_NATIVE_WRITE
		*(T*)(start + offset) = (T)value;
#else
		T tvalue = (T)value;
		uint64_t uint = 0;
		memcpy(&uint, &tvalue, sizeof(T));
		for (size_t i = 0; i < sizeof(T); i++) {
			start[offset + i] = (uint >> (i*8)) & 0xff;
		}
#endif
	}

	template <typename T, typename U>
	void write(U value) {
#if FBXDOM_NATIVE_WRITE
		*(T*)ptr = (T)value;
		ptr += sizeof(T);
#else
		T tvalue = (T)value;
		uint64_t uint = 0;
		memcpy(&uint, &tvalue, sizeof(T));
		for (size_t i = 0; i < sizeof(T); i++) {
			ptr[i] = (uint >> (i*8)) & 0xff;
		}
		ptr += sizeof(T);
#endif
	}

	void write(const char *data, size_t size)
	{
		memcpy(ptr, data, size);
		ptr += size;
	}

	void write(const std::vector<char> &data)
	{
		write(data.data(), data.size());
	}
};

struct binary_parser
{
	binary_reader reader;
	uint32_t version = 0;

	binary_parser(const void *data, size_t size)
		: reader(data, size) { }

	void parse_array(value &v, size_t elem_size)
	{
		v.data_array.length = reader.read<uint32_t>();
		v.data_array.encoding = reader.read<uint32_t>();
		v.data_array.compressed_length = reader.read<uint32_t>();
		v.data = reader.read_vector(v.data_array.compressed_length);
	}

	value parse_value()
	{
		value v;

		char type = reader.read<char>();
		v.type = type;
		switch (type) {
		case 'B': case 'C': case 'Z': v.data_float = (double)(v.data_int = reader.read<uint8_t>()); break;
		case 'Y': v.data_float = (double)(v.data_int = reader.read<int16_t>()); break;
		case 'I': v.data_float = (double)(v.data_int = reader.read<int32_t>()); break;
		case 'L': v.data_float = (double)(v.data_int = reader.read<int64_t>()); break;
		case 'F': v.data_int = (int64_t)(v.data_float = reader.read<float>()); break;
		case 'D': v.data_int = (int64_t)(v.data_float = reader.read<double>()); break;
		case 'b': case 'c': parse_array(v, 1); break;
		case 'i': case 'f': parse_array(v, 4); break;
		case 'l': case 'd': parse_array(v, 8); break;
		case 'S': case 'R': v.data = reader.read_vector(reader.read<uint32_t>()); break;
		}

		return v;
	}

	node_ptr parse_node()
	{
		size_t num_properties;
		size_t end_offset;

		if (version >= 7500) {
			end_offset = (size_t)reader.read<uint64_t>();
			num_properties = (size_t)reader.read<uint64_t>();
			reader.skip(8);
		} else {
			end_offset = (size_t)reader.read<uint32_t>();
			num_properties = (size_t)reader.read<uint32_t>();
			reader.skip(4);
		}

		size_t name_len = reader.read<uint8_t>();
		if (end_offset == 0 && name_len == 0) return { };

		node_ptr n = std::make_shared<node>();

		n->name = reader.read_vector(name_len);
		n->values.reserve(num_properties);
		for (size_t i = 0; i < num_properties; i++) {
			n->values.push_back(parse_value());
		}

		while (reader.offset() < end_offset || end_offset == 0) {
			node_ptr child = parse_node();
			if (!child) break;
			n->children.push_back(std::move(child));
		}

		return n;
	}

	node_ptr parse_root()
	{
		std::vector<char> magic = reader.read_vector(21);
		if (memcmp(magic.data(), "Kaydara FBX Binary  \x00", 21) != 0) return { };
		reader.skip(2);
		version = reader.read<uint32_t>();

		node_ptr root = std::make_shared<node>();

		{
			value v;
			v.data_int = version;
			root->values.push_back(v);
		}

		for (;;) {
			node_ptr child = parse_node();
			if (!child) break;
			root->children.push_back(std::move(child));
		}

		return root;
	}
};

struct binary_dumper
{
	binary_writer writer;
	uint32_t version = 0;

	binary_dumper(void *data, size_t size)
		: writer(data, size) { }

	void dump_array(const value &v)
	{
		writer.write<uint32_t>(v.data_array.length);
		writer.write<uint32_t>(v.data_array.encoding);
		writer.write<uint32_t>(v.data_array.compressed_length);
		writer.write(v.data);
	}

	void dump_value(const value &v)
	{
		writer.write<char>(v.type);
		switch (v.type) {
		case 'B': case 'C': writer.write<int8_t>(v.data_int); break;
		case 'Y': writer.write<int16_t>(v.data_int); break;
		case 'I': writer.write<int32_t>(v.data_int); break;
		case 'L': writer.write<int64_t>(v.data_int); break;
		case 'F': writer.write<float>(v.data_float); break;
		case 'D': writer.write<double>(v.data_float); break;
		case 'b': case 'c': dump_array(v); break;
		case 'i': case 'f': dump_array(v); break;
		case 'l': case 'd': dump_array(v); break;
		case 'S': case 'R':
			writer.write<uint32_t>(v.data.size());
			writer.write(v.data);
			break;
		}
	}

	void dump_node(node_ptr node)
	{
		size_t node_start = writer.offset();
		if (version >= 7500) {
			writer.skip(8);
			writer.write<uint64_t>(node->values.size());
			writer.skip(8);
		} else {
			writer.skip(4);
			writer.write<uint32_t>(node->values.size());
			writer.skip(4);
		}

		writer.write<uint8_t>(node->name.size());
		writer.write(node->name);

		size_t value_start = writer.offset();
		for (const value &v : node->values) {
			dump_value(v);
		}
		size_t value_end = writer.offset();

		if (!node->children.empty()) {
			for (const node_ptr child : node->children) {
				dump_node(child);
			}
			writer.skip(version >= 7500 ? 25 : 13);
		}

		size_t node_end = writer.offset();

		if (version >= 7500) {
			writer.write<uint64_t>(node_end, node_start + 0);
			writer.write<uint64_t>(value_end - value_start, node_start + 16);
		} else {
			writer.write<uint32_t>(node_end, node_start + 0);
			writer.write<uint32_t>(value_end - value_start, node_start + 8);
		}
	}

	void dump_root(node_ptr root)
	{
		if (root->values.size() > 0) {
			version = (uint32_t)root->values[0].data_int;
		} else {
			version = 7500;
		}

		writer.write("Kaydara FBX Binary  \x00\x1a\x00", 23);
		writer.write<uint32_t>(version);

		for (const node_ptr child : root->children) {
			dump_node(child);
		}
		writer.skip(version >= 7500 ? 25 : 13);
	}
};

node_ptr node::copy() const
{
	return std::make_shared<node>(*this);
}

node_ptr parse(const void *data, size_t size)
{
	return binary_parser(data, size).parse_root();
}

size_t dump(void *dst, size_t size, node_ptr root)
{
	binary_dumper d { dst, size };
	d.dump_root(root);
	return d.writer.offset();
}

}
