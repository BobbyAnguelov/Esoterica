#define _CRT_SECURE_NO_WARNINGS
#include "picort.h"
#include <algorithm>

// Load a PNG image file
// http://www.libpng.org/pub/png/spec/1.2/PNG-Contents.html
ImageResult read_png(const void *data, size_t size)
{
	Image image;

	const uint8_t *ptr = (const uint8_t*)data, *end = ptr + size;
	uint8_t bit_depth = 0, pixel_values = 0, pixel_format;
	std::vector<uint8_t> deflate_data, src, dst;
	std::vector<Pixel16> palette;
	static const uint8_t lace_none[] = { 0,0,1,1, 0,0,0,0 };
	static const uint8_t lace_adam7[] = {
		0,0,8,8, 4,0,8,8, 0,4,4,8, 2,0,4,4, 0,2,2,4, 1,0,2,2, 0,1,1,2, 0,0,0,0, };
	uint32_t scale = 1;
	const uint8_t *lace = lace_none; // Interlacing pattern (x0,y0,dx,dy)
	Pixel16 trns = { 0, 0, 0, 0 }; // Transparent pixel value for RGB

	if (end - ptr < 8) return { "file header truncated" };
	if (memcmp(ptr, "\x89PNG\r\n\x1a\n", 8)) return { "bad file header" };
	ptr += 8;

	// Iterate chunks: gather IDAT into a single buffer
	for (;;) {
		if (end - ptr < 8) return { "chunk header truncated" };
		uint32_t chunk_len = ptr[0]<<24 | ptr[1]<<16 | ptr[2]<<8 | ptr[3];
		const uint8_t *tag = ptr + 4;
		ptr += 8;
		if ((uint32_t)(end - ptr) < chunk_len + 4) return { "chunk data truncated" };
		if (!memcmp(tag, "IHDR", 4) && chunk_len >= 13) {
			image.width  = ptr[0]<<24 | ptr[1]<<16 | ptr[2]<<8 | ptr[3];
			image.height = ptr[4]<<24 | ptr[5]<<16 | ptr[6]<<8 | ptr[7];
			bit_depth = ptr[8];
			pixel_format = ptr[9];
			switch (pixel_format) {
			case 0: pixel_values = 1; break;
			case 2: pixel_values = 3; break;
			case 3: pixel_values = 1; break;
			case 4: pixel_values = 2; break;
			case 6: pixel_values = 4; break;
			default: return { "unknown pixel format" };
			}
			for (uint32_t i = 0; i < 16 && pixel_format != 3; i += bit_depth) scale |= 1u << i;
			if (ptr[12] != 0) lace = lace_adam7;
			if (ptr[10] != 0 || ptr[11] != 0) return { "unknown settings" };
		} else if (!memcmp(tag, "IDAT", 4)) {
			deflate_data.insert(deflate_data.end(), ptr, ptr + chunk_len);
		} else if (!memcmp(tag, "PLTE", 4)) {
			for (size_t i = 0; i < chunk_len; i += 3) {
				palette.push_back({ (uint16_t)(ptr[i]*0x101), (uint16_t)(ptr[i+1]*0x101), (uint16_t)(ptr[i+2]*0x101) });
			}
		} else if (!memcmp(tag, "IEND", 4)) {
			break;
		} else if (!memcmp(tag, "tRNS", 4)) {
			if (pixel_format == 2 && chunk_len >= 6) {
				trns = {
					(uint16_t)((ptr[0]<<8 | ptr[1]) * scale),
					(uint16_t)((ptr[2]<<8 | ptr[3]) * scale),
					(uint16_t)((ptr[4]<<8 | ptr[5]) * scale) };
			} else if (pixel_format == 0 && chunk_len >= 2) {
				uint16_t v = (uint16_t)(ptr[0]<<8 | ptr[1]) * scale;
				trns = { v, v, v };
			} else if (pixel_format == 3) {
				for (size_t i = 0; i < chunk_len; i++) {
					if (i < palette.size()) palette[i].a = ptr[i] * 0x101;
				}
			}
		}
		ptr += chunk_len + 4; // Skip data and CRC
	}

	size_t bpp = (pixel_values * bit_depth + 7) / 8;
	size_t stride = (image.width * pixel_values * bit_depth + 7) / 8;
	if (image.width == 0 || image.height == 0) return { "bad image size" };
	src.resize(image.height * stride + image.height * (lace == lace_adam7 ? 14 : 1));
	dst.resize((image.height + 1) * (stride + bpp));
	image.pixels.resize(image.width * image.height);

	// Decompress the combined IDAT chunks
	ufbx_inflate_retain retain;
	retain.initialized = false;

	ufbx_inflate_input input = { 0 };
	input.total_size = deflate_data.size();
	input.data_size = deflate_data.size();
	input.data = deflate_data.data();

	ptrdiff_t res = ufbx_inflate(src.data(), src.size(), &input, &retain);
	if (res < 0) return { "deflate error" };
	uint8_t *sp = src.data(), *sp_end = sp + src.size();

	for (; lace[2]; lace += 4) {
		int32_t width = ((int32_t)image.width - lace[0] + lace[2] - 1) / lace[2];
		int32_t height = ((int32_t)image.height - lace[1] + lace[3] - 1) / lace[3];
		if (width <= 0 || height <= 0) continue;
		size_t lace_stride = (width * pixel_values * bit_depth + 7) / 8;
		if ((size_t)(sp_end - sp) < height * (1 + lace_stride)) return { "data truncated" };

		// Unfilter the scanlines
		ptrdiff_t dx = bpp, dy = stride + bpp;
		for (int32_t y = 0; y < height; y++) {
			uint8_t *dp = dst.data() + (stride + bpp) * (1 + y) + bpp;
			uint8_t filter = *sp++;
			for (int32_t x = 0; x < lace_stride; x++) {
				uint8_t s = *sp++, *d = dp++;
				switch (filter) {
				case 0: d[0] = s; break; // 6.2: No filter
				case 1: d[0] = d[-dx] + s; break; // 6.3: Sub (predict left)
				case 2: d[0] = d[-dy] + s; break; // 6.4: Up (predict top)
				case 3: d[0] = (d[-dx] + d[-dy]) / 2 + s; break; // 6.5: Average (top+left)
				case 4: { // 6.6: Paeth (choose closest of 3 to estimate)
					int32_t a = d[-dx], b = d[-dy], c = d[-dx - dy], p = a+b-c;
					int32_t pa = abs(p-a), pb = abs(p-b), pc = abs(p-c);
					if (pa <= pb && pa <= pc) d[0] = (uint8_t)(a + s);
					else if (pb <= pc) d[0] = (uint8_t)(b + s);
					else d[0] = (uint8_t)(c + s);
				} break;
				default: return { "unknown filter" };
				}
			}
		}

		// Expand to RGBA pixels
		for (int32_t y = 0; y < height; y++) {
			uint8_t *dr = dst.data() + (stride + bpp) * (y + 1) + bpp;
			for (int32_t x = 0; x < width; x++) {
				uint16_t v[4];
				for (uint32_t c = 0; c < pixel_values; c++) {
					ptrdiff_t bit = (x * pixel_values + c + 1) * bit_depth;
					uint32_t raw = (dr[(bit - 9) >> 3] << 8 | dr[(bit - 1) >> 3]) >> ((8 - bit) & 7);
					v[c] = (raw & ((1 << bit_depth) - 1)) * scale;
				}

				Pixel16 px;
				switch (pixel_format) {
				case 0: px = { v[0], v[0], v[0], 0xffff }; break;
				case 2: px = { v[0], v[1], v[2], 0xffff }; break;
				case 3: px = v[0] < palette.size() ? palette[v[0]] : Pixel16{ }; break;
				case 4: px = { v[0], v[0], v[0], v[1] }; break;
				case 6: px = { v[0], v[1], v[2], v[3] }; break;
				}
				if (px == trns) px = { 0, 0, 0, 0 };
				image.pixels[(lace[1]+y*lace[3]) * image.width + (lace[0]+x*lace[2])] = px;
			}
		}

	}

	return { NULL, image };
}

std::vector<char> load_file(const char *filename)
{
	std::vector<char> data;
	FILE *f = fopen(filename, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		data.resize(size);
		size_t result = fread(data.data(), 1, size, f);
		ufbx_assert(result == size);
		fclose(f);
	}
	return data;
}

ImageResult load_png(const char *filename)
{
	std::vector<char> data = load_file(filename);
	return read_png(data.data(), data.size());
}

// -- DEFLATE

struct HuffSymbol {
	uint16_t length, bits;
};

uint32_t bit_reverse(uint32_t mask, uint32_t num_bits)
{
	ufbx_assert(num_bits <= 16);
	uint32_t x = mask;
	x = (((x & 0xaaaa) >> 1) | ((x & 0x5555) << 1));
	x = (((x & 0xcccc) >> 2) | ((x & 0x3333) << 2));
	x = (((x & 0xf0f0) >> 4) | ((x & 0x0f0f) << 4));
	x = (((x & 0xff00) >> 8) | ((x & 0x00ff) << 8));
	return x >> (16 - num_bits);
}

void build_huffman(HuffSymbol *symbols, uint32_t *counts, uint32_t num_symbols, uint32_t max_bits)
{
	struct HuffNode {
		uint16_t index; uint16_t parent; uint32_t count;
		bool operator<(const HuffNode &rhs) const {
			return count != rhs.count ? count < rhs.count : index > rhs.index;
		}
	};
	HuffNode nodes[1024];

	uint32_t bias = 0;
	for (;;) {
		for (uint32_t i = 0; i < num_symbols; i++) {
			nodes[i] = { (uint16_t)i, UINT16_MAX, counts[i] ? counts[i] + bias : 0 };
		}
		std::sort(nodes, nodes + num_symbols);
		uint32_t cs = 0, ce = num_symbols, qs = 512, qe = qs;
		while (cs + 2 < ce && nodes[cs].count == 0) cs++;
		while (ce-cs + qe-qs > 1) {
			uint32_t a = ce-cs > 0 && (qe-qs == 0 || nodes[cs].count < nodes[qs].count) ? cs++ : qs++;
			uint32_t b = ce-cs > 0 && (qe-qs == 0 || nodes[cs].count < nodes[qs].count) ? cs++ : qs++;
			nodes[a].parent = nodes[b].parent = (uint16_t)qe;
			nodes[qe++] = { UINT16_MAX, UINT16_MAX, nodes[a].count + nodes[b].count };
		}

		bool fail = false;
		uint32_t length_counts[16] = { }, length_codes[16] = { };
		for (uint32_t i = 0; i < num_symbols; i++) {
			uint32_t len = 0;
			for (uint32_t a = nodes[i].parent; a != UINT16_MAX; a = nodes[a].parent) len++;
			if (len > max_bits) { fail = true; break; }
			length_counts[len]++;
			symbols[nodes[i].index].length = (uint16_t)len;
		}
		if (fail) {
			bias = bias ? bias << 1 : 1;
			continue;
		}

		uint32_t code = 0, prev_count = 0;
		for (uint32_t bits = 1; bits < 16; bits++) {
			uint32_t count = length_counts[bits];
			code = (code + prev_count) << 1;
			prev_count = count;
			length_codes[bits] = code;
		}

		for (uint32_t i = 0; i < num_symbols; i++) {
			uint32_t len = symbols[i].length;
			symbols[i].bits = len ? bit_reverse(length_codes[len]++, len) : 0;
		}

		break;
	}
}

uint32_t match_len(const unsigned char *a, const unsigned char *b, uint32_t max_length)
{
	if (max_length > 258) max_length = 258;
	for (uint32_t len = 0; len < max_length; len++) {
		if (*a++ != *b++) return len;
	}
	return max_length;
}

uint16_t encode_lit(uint32_t value, uint32_t bits)
{
	return (uint16_t)(value | (0xfffeu << bits));
}

uint32_t sym_offset_bits(HuffSymbol *syms, uint32_t code, uint32_t base, const uint16_t *counts, uint32_t value)
{
	for (uint32_t bits = 0; counts[bits]; bits++) {
		uint32_t num = counts[bits] << bits, delta = value - base;
		if (delta < num) {
			return syms[code + (delta >> bits)].length + bits;
		}
		code += counts[bits];
		base += num;
	}
	return syms[code].bits;
}

size_t encode_sym_offset(uint16_t *dst, uint16_t code, uint32_t base, const uint16_t *counts, uint32_t value)
{
	for (uint32_t bits = 0; counts[bits]; bits++) {
		uint32_t num = counts[bits] << bits, delta = value - base;
		if (delta < num) {
			dst[0] = code + (delta >> bits);
			if (bits > 0) {
				dst[1] = encode_lit(delta & ((1 << bits) - 1), bits);
				return 2;
			} else {
				return 1;
			}
		}
		code += counts[bits];
		base += num;
	}
	dst[0] = code;
	return 1;
}

uint32_t lz_hash(const unsigned char *d)
{
	uint32_t x = (uint32_t)d[0] | (uint32_t)d[1] << 8 | (uint32_t)d[2] << 16;
	x ^= x >> 16;
	x *= UINT32_C(0x7feb352d);
	x ^= x >> 15;
	x *= UINT32_C(0x846ca68b);
	x ^= x >> 16;
	return x ? x : 1;
}

void init_tri_dist(uint16_t *tri_dist, const void *data, uint32_t length)
{
	const uint32_t max_scan = 32;
	const unsigned char *d = (const unsigned char*)data;
	struct Match {
		uint32_t hash = 0;
		uint32_t offset = 0x80000000;
	};
	std::vector<Match> match_table;
	uint32_t mask = 0x1ffff;
	match_table.resize(mask + 1);
	for (uint32_t i = 0; i < length; i++) {
		if (length - i >= 3) {
			uint32_t hash = lz_hash(d + i), replace_ix = 0, replace_score = 0;
			for (uint32_t scan = 0; scan < max_scan; scan++) {
				uint32_t ix = (hash + scan) & mask;
				uint32_t delta = (uint32_t)(i - match_table[ix].offset);
				if (match_table[ix].hash == hash && delta <= 32768) {
					tri_dist[i] = (uint16_t)delta;
					match_table[ix].offset = i;
					replace_ix = UINT32_MAX;
					break;
				} else {
					uint32_t score = delta <= 32768 ? delta : 32768 + max_scan - scan;
					if (score > replace_score) {
						replace_score = score;
						replace_ix = ix;
						if (score > 32768) break;
					}
				}
			}
			if (replace_ix != UINT32_MAX) {
				match_table[replace_ix].hash = hash;
				match_table[replace_ix].offset = i;
				tri_dist[i] = UINT16_MAX;
			}
		} else {
			tri_dist[i] = UINT16_MAX;
		}
	}
}

static picort_forceinline bool cmp3(const void *a, const void *b)
{
	const char *ca = (const char*)a, *cb = (const char*)b;
	return ca[0] == cb[0] && ca[1] == cb[1] && ca[2] == cb[2];
}

struct LzMatch
{
	uint32_t len = 0, dist = 0, bits = 0;
};

static const uint16_t len_counts[] = { 8, 4, 4, 4, 4, 4, 0 };
static const uint16_t dist_counts[] = { 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0 };

LzMatch find_match(HuffSymbol *syms, const uint16_t *tri_dist, const void *data, uint32_t begin, uint32_t end)
{
	const unsigned char *d = (const unsigned char*)data;
	int32_t max_len = (int32_t)end - begin - 1;
	if (max_len > 258) max_len = 258;
	if (max_len < 3) return { };
	uint32_t best_len = 0, best_dist = 0;
	int32_t m_off = begin - tri_dist[begin], m_end = m_off, m_delta = 0;
	uint32_t best_bits = syms[d[begin + 0]].length + syms[d[begin + 1]].length;
	for (size_t scan = 0; scan < 1000; scan++) {
		if (begin - m_off > 32768 || m_off < 0 || m_end < 0 || m_delta > max_len - 3) break;
		int32_t delta = m_end - m_off - 3;

		bool ok = true;
		if (m_off >= 0 && (m_end - m_off < m_delta || !cmp3(d + m_off + m_delta, d + begin + m_delta))) {
			m_off -= tri_dist[m_off];
			if (m_off >= 0 && m_end - m_off > m_delta && cmp3(d + m_off + m_delta, d + begin + m_delta)) {
				m_end = m_off + m_delta;
			}
			ok = false;
		}
		if (m_end >= m_delta && (m_end - m_off < m_delta || !cmp3(d + m_end - m_delta, d + begin))) {
			m_end -= tri_dist[m_end];
			if (m_end >= m_delta && m_end - m_off < m_delta && cmp3(d + m_end - m_delta, d + begin)) {
				m_off = m_end - m_delta;
			}
			ok = false;
		}
		if (!ok) continue;

		if (m_delta <= 3 || !memcmp(d + begin, d + m_off, m_delta + 3)) {
			do {
				uint32_t m_len = m_delta + 3, m_dist = begin - m_off;
				uint32_t m_bits = sym_offset_bits(syms, 257, 3, len_counts, m_len)
					+ sym_offset_bits(syms, 286, 1, dist_counts, m_dist);

				best_bits += syms[d[begin + m_len - 1]].length;
				if (m_bits < best_bits) {
					best_bits = m_bits;
					best_len = m_len;
					best_dist = m_dist;
				}

				m_end++;
				m_delta++;
			} while (m_delta <= max_len - 3 && d[begin + m_delta + 2] == d[m_off + m_delta + 2]);
		} else {
			m_off--;
		}
	}
	return { best_len, best_dist, best_bits };
}

uint32_t encode_lz(HuffSymbol *syms, uint16_t *dst, const uint16_t *tri_dist, const void *data, uint32_t begin, uint32_t end, uint32_t *p_bits)
{
	const unsigned char *d = (const unsigned char*)data;
	uint16_t *p = dst;
	uint32_t bits = 0;
	for (int32_t i = (int32_t)begin; i < (int32_t)end; i++) {

		LzMatch match = find_match(syms, tri_dist, data, i, end);
		while (match.len > 0) {
			LzMatch next = find_match(syms, tri_dist, data, i + 1, end);
			if (next.len == 0) break;

			uint32_t match_bits = match.bits, next_bits = next.bits + syms[d[i]].length;
			for (uint32_t j = i + match.len; j < i + 1 + next.len; j++) match_bits += syms[d[j]].length;
			for (uint32_t j = i + 1 + next.len; j < i + match.len; j++) next_bits += syms[d[j]].length;
			if (next_bits >= match_bits) break;

			bits += syms[d[i]].length;
			*p++ = d[i++];
			match = next;
		}

		if (match.len > 0) {
			assert(!memcmp(d + i, d + i - match.dist, match.len));
			p += encode_sym_offset(p, 257, 3, len_counts, match.len);
			p += encode_sym_offset(p, 286, 1, dist_counts, match.dist);
			i += match.len - 1;
		} else {
			*p++ = d[i];
		}
	}
	return (uint32_t)(p - dst);
}

size_t encode_lengths(uint16_t *dst, HuffSymbol *syms, uint32_t *p_count, uint32_t min_count)
{
	uint32_t count = *p_count;
	while (count > min_count && syms[count - 1].length == 0) count--;
	*p_count = count;

	uint16_t *p = dst;
	for (uint32_t begin = 0; begin < count; ) {
		uint16_t len = syms[begin].length;
		uint32_t end = begin + 1;
		while (end < count && syms[end].length == len) {
			end++;
		}

		uint32_t span_begin = begin;
		while (begin < end) {
			uint32_t num = end - begin;
			if (num < 3 || (len > 0 && begin == span_begin)) {
				num = 1;
				*p++ = len;
			} else if (len == 0 && end - begin < 11) {
				*p++ = 17;
				*p++ = encode_lit(num - 3, 3);
			} else if (len == 0) {
				if (num > 138) num = 138;
				*p++ = 18;
				*p++ = encode_lit(num - 11, 7);
			} else { 
				if (num > 6) num = 6;
				*p++ = 16;
				*p++ = encode_lit(num - 3, 2);
			}
			begin += num;
		}
	}
	return (size_t)(p - dst);
}

size_t flush_bits(char **p_dst, size_t reg, size_t num)
{
	char *dst = *p_dst;
	for (; num >= 8; num -= 8) {
		*dst++ = (uint8_t)reg;
		reg >>= 8;
	}
	*p_dst = dst;
	return reg;
}

picort_forceinline void push_bits(char **p_dst, size_t *p_reg, size_t *p_num, uint32_t value, uint32_t bits)
{
	if (*p_num + bits > sizeof(size_t) * 8) {
		*p_reg = flush_bits(p_dst, *p_reg, *p_num);
		*p_num &= 0x7;
	}
	*p_reg |= (size_t)value << *p_num;
	*p_num += bits;
}

void encode_syms(char **p_dst, size_t *p_reg, size_t *p_num, HuffSymbol *table, const uint16_t *syms, size_t num_syms)
{
	size_t reg = *p_reg, num = *p_num;
	for (size_t i = 0; i < num_syms; i++) {
		uint32_t sym = syms[i];
		if (sym & 0x8000) {
			// TODO: BSR?
			uint32_t bits = 15;
			while (sym & (1 << bits)) bits--;
			push_bits(p_dst, &reg, &num, sym & ((1 << bits) - 1), bits);
		} else {
			HuffSymbol hs = table[sym];
			push_bits(p_dst, &reg, &num, hs.bits, hs.length);
		}
	}
	*p_reg = reg;
	*p_num = num;
}

uint32_t adler32(const void *data, size_t size)
{
	size_t a = 1, b = 0;
	const char *p = (const char*)data;
	const size_t num_before_wrap = sizeof(size_t) == 8 ? 380368439u : 5552u;
	size_t size_left = size;
	while (size_left > 0) {
		size_t num = size_left <= num_before_wrap ? size_left : num_before_wrap;
		size_left -= num;
		const char *end = p + num;
		while (p != end) {
			a += (size_t)(uint8_t)*p++; b += a;
		}
		a %= 65521u;
		b %= 65521u;
	}
	return (uint32_t)((b << 16) | (a & 0xffff));
}

size_t deflate(void *dst, const void *data, size_t length)
{
	char *dp = (char*)dst;

	static const uint32_t lz_block_size = 8*1024*1024;
	static const uint32_t huff_block_size = 64*1024;

	std::vector<uint16_t> sym_buf, tri_buf;
	sym_buf.resize(std::min((size_t)huff_block_size, length));
	tri_buf.resize(std::min((size_t)lz_block_size, length));

	size_t reg = 0, num = 0;

	push_bits(&dp, &reg, &num, 0x78, 8);
	push_bits(&dp, &reg, &num, 0x9c, 8);

	for (size_t lz_base = 0; lz_base < length; lz_base += lz_block_size) {
		size_t lz_length = length - lz_base;
		if (lz_length > lz_block_size) lz_length = lz_block_size;

		init_tri_dist(tri_buf.data(), (const char*)data + lz_base, (uint32_t)lz_length);

		for (size_t huff_base = 0; huff_base < lz_length; huff_base += huff_block_size) {
			size_t huff_length = lz_length - huff_base;
			if (huff_length > huff_block_size) huff_length = huff_block_size;

			uint16_t *syms = sym_buf.data();
			size_t num_syms = 0;

			HuffSymbol sym_huffs[316];
			for (uint32_t i = 0; i < 316; i++) {
				sym_huffs[i].length = i >= 256 ? 6 : 8;
			}

			for (uint32_t i = 0; i < 2; i++) {
				uint32_t bits = 0;
				num_syms = encode_lz(sym_huffs, syms, tri_buf.data(), (const char*)data + lz_base, (uint32_t)huff_base, (uint32_t)(huff_base + huff_length), &bits);

				uint32_t sym_counts[316] = { };
				for (size_t i = 0; i < num_syms; i++) {
					uint32_t sym = syms[i];
					if (sym < 316) sym_counts[sym]++;
				}
				sym_counts[256] = 1;

				build_huffman(sym_huffs + 0, sym_counts + 0, 286, 15);
				build_huffman(sym_huffs + 286, sym_counts + 286, 30, 15);
			}

			uint32_t hlit = 286, hdist = 30;
			uint16_t header_syms[316], *header_dst = header_syms;
			header_dst += encode_lengths(header_dst, sym_huffs + 0, &hlit, 257);
			header_dst += encode_lengths(header_dst, sym_huffs + 286, &hdist, 1);
			size_t header_len = (size_t)(header_dst - header_syms);

			uint32_t header_counts[19] = { };
			HuffSymbol header_huffs[19];
			for (size_t i = 0; i < header_len; i++) {
				uint32_t sym = header_syms[i];
				if (sym < 19) header_counts[sym]++;
			}

			build_huffman(header_huffs, header_counts, 19, 7);
			static const uint8_t lens[] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
			uint32_t hclen = 19;
			while (hclen > 4 && header_huffs[lens[hclen - 1]].length == 0) hclen--;

			bool end = huff_base + huff_length == lz_length && lz_base + lz_length == length;
			push_bits(&dp, &reg, &num, end ? 0x5 : 0x4, 3);

			push_bits(&dp, &reg, &num, hlit - 257, 5);
			push_bits(&dp, &reg, &num, hdist - 1, 5);
			push_bits(&dp, &reg, &num, hclen - 4, 4);
			for (uint32_t i = 0; i < hclen; i++) {
				push_bits(&dp, &reg, &num, header_huffs[lens[i]].length, 3);
			}

			encode_syms(&dp, &reg, &num, header_huffs, header_syms, header_len);
			encode_syms(&dp, &reg, &num, sym_huffs, syms, num_syms);

			HuffSymbol end_hs = sym_huffs[256];
			push_bits(&dp, &reg, &num, end_hs.bits, end_hs.length);
		}
	}

	if (num % 8 != 0) push_bits(&dp, &reg, &num, 0, 8 - (num % 8));
	uint32_t checksum = adler32(data, length);
	for (size_t i = 0; i < 4; i++) {
		push_bits(&dp, &reg, &num, (checksum >> (24 - i * 8)) & 0xff, 8);
	}

	flush_bits(&dp, reg, num);

	return (size_t)(dp - (char*)dst);
}

void png_filter_row(uint32_t method, uint8_t *dst, const uint8_t *line, const uint8_t *prev, uint32_t width, uint32_t num_channels)
{
	int32_t dx = (int32_t)num_channels, pitch = (int32_t)(width * num_channels);
	if (method == 0) {
		for (int32_t i = 0; i < pitch; i++) dst[i] = line[i];
	} else if (method == 1) {
		for (int32_t i = 0; i < pitch; i++) dst[i] = line[i] - line[i-dx];
	} else if (method == 2) {
		for (int32_t i = 0; i < pitch; i++) dst[i] = line[i] - prev[i];
	} else if (method == 3) {
		for (int32_t i = 0; i < pitch; i++) dst[i] = line[i] - (line[i-dx] + prev[i]) / 2;
	} else if (method == 4) {
		for (int32_t i = 0; i < pitch; i++) {
			int32_t s = line[i], a = line[i-dx], b = prev[i], c = prev[i-dx], p = a+b-c;
			int32_t pa = abs(p-a), pb = abs(p-b), pc = abs(p-c);
			if (pa <= pb && pa <= pc) dst[i] = (uint8_t)(s - a);
			else if (pb <= pc) dst[i] = (uint8_t)(s - b);
			else dst[i] = (uint8_t)(s - c);
		}
	}
}

void crc32_init(uint32_t *crc_table)
{
	for (uint32_t i = 0; i < 256; i++) {
		uint32_t r = i;
		for(uint32_t k = 0; k < 8; ++k) {
			r = ((r & 1) ? UINT32_C(0xEDB88320) : 0) ^ (r >> 1);
		}
		crc_table[i] = r;
	}
}

uint32_t crc32(uint32_t *crc_table, const void *data, size_t size, uint32_t seed)
{
	uint32_t crc = ~seed;
	const uint8_t *src = (const uint8_t*)data;
	for (size_t i = 0; i < size; i++) {
		crc = crc_table[(crc ^ src[i]) & 0xff] ^ (crc >> 8);
	}
	return ~crc;
}

void png_add_chunk(uint32_t *crc_table, std::vector<uint8_t> &dst, const char *tag, const void *data, size_t size)
{
	uint8_t be_size[] = { (uint8_t)(size>>24), (uint8_t)(size>>16), (uint8_t)(size>>8), (uint8_t)size };
	dst.insert(dst.end(), be_size, be_size + 4);
	dst.insert(dst.end(), (const uint8_t*)tag, (const uint8_t*)tag + 4);
	dst.insert(dst.end(), (const uint8_t*)data, (const uint8_t*)data + size);
	uint32_t crc = crc32(crc_table, dst.data() + dst.size() - (size + 4), size + 4, 0);
	uint8_t be_crc[] = { (uint8_t)(crc>>24), (uint8_t)(crc>>16), (uint8_t)(crc>>8), (uint8_t)crc };
	dst.insert(dst.end(), be_crc, be_crc + 4);
}

std::vector<uint8_t> write_png(const void *data, uint32_t width, uint32_t height)
{
	std::vector<uint8_t> result;

	const uint8_t *pixels = (const uint8_t*)data;
	size_t num_pixels = (size_t)width * (size_t)height;

	uint32_t num_channels = 3;
	for (size_t i = 0; i < num_pixels; i++) {
		if (pixels[i * 4 + 3] < 255) {
			num_channels = 4;
			break;
		}
	}

	std::vector<uint8_t> lines[2];
	lines[0].resize((width + 1) * num_channels);
	lines[1].resize((width + 1) * num_channels);
	uint8_t *prev = lines[0].data() + num_channels;
	uint8_t *line = lines[1].data() + num_channels;

	std::vector<uint8_t> idat, idat_deflate;
	idat.resize((width * num_channels + 1) * height);
	// TODO: Proper bound...
	idat_deflate.resize(idat.size() + idat.size() / 10);

	uint32_t pitch = width * num_channels;
	uint8_t *dst = idat.data();
	for (uint32_t y = 0; y < height; y++) {
		const uint8_t *src = pixels + y * width * 4;
		for (uint32_t c = 0; c < num_channels; c++) {
			for (uint32_t x = 0; x < width; x++) {
				line[x*num_channels + c] = src[x*4 + c];
			}
		}

		uint32_t best_filter = 0, best_entropy = UINT32_MAX;
		for (uint32_t f = 0; f <= 4; f++) {
			png_filter_row(f, dst + 1, line, prev, width, num_channels);
			uint32_t entropy = 0;
			for (uint32_t x = 0; x < pitch; x++) {
				entropy += abs((int8_t)dst[1 + x]);
			}
			if (entropy < best_entropy) {
				best_filter = f;
				best_entropy = entropy;
			}
		}
		if (best_filter != 4) {
			png_filter_row(best_filter, dst + 1, line, prev, width, num_channels);
		}
		dst[0] = (uint8_t)best_filter;

		dst += width * num_channels + 1;
		std::swap(prev, line);
	}

	size_t idat_length = deflate(idat_deflate.data(), idat.data(), idat.size());

	uint8_t ihdr[] = {
		(uint8_t)(width>>24), (uint8_t)(width>>16), (uint8_t)(width>>8), (uint8_t)width,
		(uint8_t)(height>>24), (uint8_t)(height>>16), (uint8_t)(height>>8), (uint8_t)height,
		8, (uint8_t)(num_channels == 4 ? 6 : 2), 0, 0, 0,
	};

	uint32_t crc_table[256];
	crc32_init(crc_table);

	const char magic[] = "\x89PNG\r\n\x1a\n";
	result.insert(result.end(), magic, magic + 8);
	png_add_chunk(crc_table, result, "IHDR", ihdr, sizeof(ihdr));
	png_add_chunk(crc_table, result, "IDAT", idat_deflate.data(), idat_length);
	png_add_chunk(crc_table, result, "IEND", NULL, 0);
	return result;
}

void save_png(const char *filename, const void *data, uint32_t width, uint32_t height, int frame)
{
	std::vector<char> name;
	for (const char *c = filename; *c;) {
		if (*c != '#') {
			name.push_back(*c++);
		} else {
			int width = 0;
			while (*c == '#') {
				width++;
				c++;
			}

			if (width > 0) {
				char tmp[64];
				int len = snprintf(tmp, sizeof(tmp), "%0*d", width, frame);
				name.insert(name.end(), tmp, tmp + len);
			}
		}
	}
	name.push_back('\0');

	std::vector<uint8_t> png = write_png(data, width, height);

	bool write_fail = false;
	FILE *f = fopen(name.data(), "wb");
	if (f) {
		if (fwrite(png.data(), 1, png.size(), f) != png.size()) write_fail = true;
		if (fclose(f) != 0) write_fail = true;
	} else {
		write_fail = true;
	}

	if (write_fail) {
		fprintf(stderr, "Failed to save file: %s\n", name.data());
		exit(1);
	}
}
