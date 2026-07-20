#define _CRT_SECURE_NO_WARNINGS

#include <zlib.h>

#define CPUTIME_IMPLEMENTATION
#include "../../test/cputime.h"
#include "../../ufbx.h"

#include <vector>
#include <assert.h>
#include <algorithm>
#include <stdlib.h>

#define UFBX_RETAIN 1

int inflate_memory(const void *src, int srcLen, void *dst, int dstLen) {
    z_stream strm  = {0};
    strm.total_in  = strm.avail_in  = srcLen;
    strm.total_out = strm.avail_out = dstLen;
    strm.next_in   = (Bytef *) src;
    strm.next_out  = (Bytef *) dst;

    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    int err = -1;
    int ret = -1;

    err = inflateInit2(&strm, 15);
    if (err == Z_OK) {
        err = inflate(&strm, Z_FINISH);
        if (err == Z_STREAM_END) {
            ret = strm.total_out;
        }
        else {
             inflateEnd(&strm);
             return err;
        }
    }
    else {
        inflateEnd(&strm);
        return err;
    }

    inflateEnd(&strm);
    return ret;
}

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

struct deflate_stream
{
    const void *data;
    size_t compressed_size;
    size_t decompressed_size;

    uint64_t zlib_time = UINT64_MAX;
    uint64_t ufbx_time = UINT64_MAX;
};

int main(int argc, char **argv)
{
	std::vector<char> data = read_file(argv[1]);
	std::vector<deflate_stream> streams;
	std::vector<char> dst_buf;

    cputime_begin_init();

    size_t max_decompressed_size = 0;

    for (size_t offset = 0; offset < data.size(); ) {
        deflate_stream stream;
        stream.compressed_size = *(uint32_t*)(data.data() + offset + 0);
        stream.decompressed_size = *(uint32_t*)(data.data() + offset + 4);
        stream.data = data.data() + offset + 8;
        offset += 8 + stream.compressed_size;
        if (stream.decompressed_size > max_decompressed_size) {
            max_decompressed_size = stream.decompressed_size;
        }
        streams.push_back(stream);
    }

    if (argc > 2) {
        deflate_stream single = streams[atoi(argv[2])];
        streams.clear();
        streams.push_back(single);
    }

    size_t runs = 3;
    if (argc > 3) {
        runs = (size_t)atoi(argv[3]);
    }

    dst_buf.resize(max_decompressed_size);

    for (size_t i = 0; i < runs; i++) {
		for (deflate_stream &stream : streams) {
			uint64_t begin = cputime_cpu_tick();

			int ret = inflate_memory(stream.data, (int)stream.compressed_size,
				dst_buf.data(), (int)stream.decompressed_size);
			assert(ret == stream.decompressed_size);

			uint64_t end = cputime_cpu_tick();

            if (argc > 20) {
                fwrite(dst_buf.data(), 1, stream.decompressed_size, stdout);
            }

			stream.zlib_time = std::min(stream.zlib_time, end - begin);
		}

#if UFBX_RETAIN
		ufbx_inflate_retain retain;
		retain.initialized = false;
#endif

		for (deflate_stream &stream : streams) {
			uint64_t begin = cputime_cpu_tick();

#if !UFBX_RETAIN
			ufbx_inflate_retain retain;
			retain.initialized = false;
#endif

			ufbx_inflate_input input = { };
			input.data_size = input.total_size = stream.compressed_size;
			input.data = stream.data;
			ptrdiff_t ret = ufbx_inflate(dst_buf.data(), stream.decompressed_size, &input, &retain);
			assert(ret == stream.decompressed_size);

            if (argc > 20) {
                fwrite(dst_buf.data(), 1, stream.decompressed_size, stdout);
            }

			uint64_t end = cputime_cpu_tick();
			stream.ufbx_time = std::min(stream.ufbx_time, end - begin);
		}
    }

    cputime_end_init();

    uint32_t index = 0;
    for (deflate_stream &stream : streams) {
        double ufbx_sec = cputime_cpu_delta_to_sec(NULL, stream.ufbx_time);
        double zlib_sec = cputime_cpu_delta_to_sec(NULL, stream.zlib_time);
        double ufbx_cbp = (double)stream.ufbx_time / (double)stream.decompressed_size;
        double zlib_cbp = (double)stream.zlib_time / (double)stream.decompressed_size;

        printf("[%6.2f] %3u: %10zu -> %10zu bytes: %8.4fms (%6.2fcy/b) vs %8.4fms (%6.2fcy/b)\n",
            ufbx_sec / zlib_sec * 100.0, index,
            stream.compressed_size, stream.decompressed_size,
            ufbx_sec*1e3, ufbx_cbp, zlib_sec*1e3, zlib_cbp);
        index++;
    }

    return 0;
}
