#define _CRT_SECURE_NO_WARNINGS

#include "../../test/domfuzz/fbxdom.h"
#include <vector>
#include <string>
#include <stdio.h>

#ifdef _WIN32
	#include <fcntl.h>
	#include <io.h>
#endif

std::vector<char> read_file(const char *path)
{
	FILE *f = fopen(path, "rb");
	fseek(f, 0, SEEK_END);
	std::vector<char> data;
	data.resize(ftell(f));
	fseek(f, 0, SEEK_SET);
	fread(data.data(), 1, data.size(), f);
	fclose(f);
	return data;
}

void dump_deflate_arrays(fbxdom::node_ptr node)
{
	for (fbxdom::value &value : node->values) {
		if (value.data_array.encoding == 1) {
			uint32_t decompressed_size = 0;
			switch (value.type) {
			case 'c': case 'b': decompressed_size = value.data_array.length * 1; break;
			case 'i': case 'f': decompressed_size = value.data_array.length * 4; break;
			case 'l': case 'd': decompressed_size = value.data_array.length * 8; break;
			}

			fwrite(&value.data_array.compressed_length, 4, 1, stdout);
			fwrite(&decompressed_size, 4, 1, stdout);

			fwrite(value.data.data(), 1, value.data.size(), stdout);
		}
	}

	for (fbxdom::node_ptr &child : node->children) {
		dump_deflate_arrays(child);
	}
}

int main(int argc, char **argv)
{
	std::vector<char> data = read_file(argv[1]);
	fbxdom::node_ptr root = fbxdom::parse(data.data(), data.size());
	if (!root) return 0;

#ifdef _WIN32
	_setmode(_fileno(stdout), O_BINARY);
#endif

	dump_deflate_arrays(root);

	return 0;
}
