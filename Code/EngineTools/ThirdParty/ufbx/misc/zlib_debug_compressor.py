from collections import namedtuple, defaultdict
import itertools

class Options:
    def __init__(self, **kwargs):
        self.override_litlen_counts = kwargs.get("override_litlen_counts", { })
        self.override_dist_counts = kwargs.get("override_dist_counts", { })
        self.max_uncompressed_length = kwargs.get("max_uncompressed_length", 0xffff)
        self.prune_interval = kwargs.get("prune_interval", 65536)
        self.max_match_distance = kwargs.get("max_match_distance", 32768)
        self.search_budget = kwargs.get("search_budget", 4096)
        self.force_block_types = kwargs.get("force_block_types", [])
        self.block_size = kwargs.get("block_size", 32768)
        self.invalid_sym = kwargs.get("invalid_sym", None)
        self.no_decode = kwargs.get("no_decode", False)

Code = namedtuple("Code", "code bits")
IntCoding = namedtuple("IntCoding", "symbol base bits")
BinDesc = namedtuple("BinDesc", "offset value bits desc")
SymExtra = namedtuple("Code", "symbol extra bits")

null_code = Code(0,0)

def make_int_coding(first_symbol, first_value, bit_sizes):
    symbol = first_symbol
    value = first_value
    codings = []
    for bits in bit_sizes:
        codings.append(IntCoding(symbol, value, bits))
        value += 1 << bits
        symbol += 1
    return codings

length_coding = make_int_coding(257, 3, [
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,
])

distance_coding = make_int_coding(0, 1, [
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,
])

def find_int_coding(codes, value):
    for coding in codes:
        if value < coding.base + (1 << coding.bits):
            return coding

class BitBuf:
    def __init__(self):
        self.pos = 0
        self.data = 0
        self.desc = []

    def push(self, val, bits, desc=""):
        if bits == 0: return
        assert val < 1 << bits
        val = int(val)
        self.desc.append(BinDesc(self.pos, val, bits, desc))
        self.data |= val << self.pos
        self.pos += bits

    def push_rev(self, val, bits, desc=""):
        if bits == 0: return
        assert val < 1 << bits
        rev = 0
        for n in range(bits):
            rev |= ((val >> n) & 1) << bits-n-1
        self.push(rev, bits, desc)

    def push_code(self, code, desc=""):
        self.push(code.code, code.bits, desc)
    def push_rev_code(self, code, desc=""):
        if code is None:
            raise RuntimeError("Empty code")
        self.push_rev(code.code, code.bits, desc)

    def append(self, buf):
        for desc in buf.desc:
            self.desc.append(desc._replace(offset = desc.offset + self.pos))
        self.data |= buf.data << self.pos
        self.pos += buf.pos

    def patch(self, offset, value, bits, desc=""):
        self.data = self.data & ~(((1 << bits) - 1) << offset) | (value << offset)

    def to_bytes(self):
        return bytes((self.data>>p&0xff) for p in range(0, self.pos, 8))

class Literal:
    def __init__(self, data):
        self.data = data
        self.length = len(data)

    def count_codes(self, litlen_count, dist_count):
        for c in self.data:
            litlen_count[c] += 1

    def encode(self, buf, litlen_syms, dist_syms, opts):
        for c in self.data:
            sym = litlen_syms.get(c, opts.invalid_sym)
            if c >= 32 and c <= 128:
                buf.push_rev_code(sym, "Literal '{}' (0x{:02x})".format(chr(c), c))
            else:
                buf.push_rev_code(sym, "Literal {:3d} (0x{:02x})".format(c, c))

    def decode(self, result):
        result += self.data

    def split(self, pos):
        assert pos >= 0
        return Literal(self.data[:pos]), Literal(self.data[pos:])

    def __repr__(self):
        return "Literal({!r})".format(self.data)

class Match:
    def __init__(self, length, distance):
        self.length = length
        self.distance = distance
        if length < 258:
            self.lcode = find_int_coding(length_coding, length)
        else:
            assert length == 258
            self.lcode = IntCoding(285, 0, 0)
        self.dcode = find_int_coding(distance_coding, distance)

    def count_codes(self, litlen_count, dist_count):
        litlen_count[self.lcode.symbol] += 1
        dist_count[self.dcode.symbol] += 1

    def encode(self, buf, litlen_syms, dist_syms, opts):
        lsym = litlen_syms.get(self.lcode.symbol, opts.invalid_sym)
        dsym = dist_syms.get(self.dcode.symbol, opts.invalid_sym)
        buf.push_rev_code(lsym, "Length: {}".format(self.length))
        if self.lcode.bits > 0:
            buf.push(self.length - self.lcode.base, self.lcode.bits, "Length extra")
        buf.push_rev_code(dsym, "Distance: {}".format(self.distance))
        if self.dcode.bits > 0:
            buf.push(self.distance - self.dcode.base, self.dcode.bits, "Distance extra")

    def decode(self, result):
        begin = len(result) - self.distance
        assert begin >= 0
        for n in range(begin, begin + self.length):
            result.append(result[n])

    def split(self, pos):
        return self, Literal(b"")

    def __repr__(self):
        return "Match({}, {})".format(self.length, self.distance)

def make_huffman_bits(syms, max_code_length):
    if len(syms) == 0:
        return { }
    if len(syms) == 1:
        return { next(iter(syms)): 1 }

    sym_groups = ((prob, (sym,)) for sym,prob in syms.items())
    initial_groups = list(sorted(sym_groups))
    groups = initial_groups

    for n in range(max_code_length-1):
        packaged = [(a[0]+b[0], a[1]+b[1]) for a,b in zip(groups[0::2], groups[1::2])]
        groups = list(sorted(packaged + initial_groups))

    sym_bits = { }
    for g in groups[:(len(syms) - 1) * 2]:
        for sym in g[1]:
            sym_bits[sym] = sym_bits.get(sym, 0) + 1
    return sym_bits

def make_huffman_codes(sym_bits, max_code_length):
    if len(sym_bits) == 0:
        return { }
    
    bl_count = [0] * (max_code_length + 1)
    next_code = [0] * (max_code_length + 1)
    for bits in sym_bits.values():
        bl_count[bits] += 1
    code = 0
    for n in range(1, max_code_length + 1):
        code = (code + bl_count[n - 1]) << 1
        next_code[n] = code

    codes = { }
    for sym,bits in sorted(sym_bits.items()):
        codes[sym] = Code(next_code[bits], bits)
        next_code[bits] += 1

    return codes

def make_huffman(syms, max_code_length):
    sym_bits = make_huffman_bits(syms, max_code_length)
    return make_huffman_codes(sym_bits, max_code_length)

def decode(message):
    result = []
    for m in message:
        m.decode(result)
    return bytes(result)

def encode_huff_bits(bits):
    encoded = []
    for value,copies in itertools.groupby(bits):
        num = len(list(copies))
        assert value < 16
        if value == 0:
            while num >= 11:
                amount = min(num, 138)
                encoded.append(SymExtra(18, amount-11, 7))
                num -= amount
            while num >= 3:
                amount = min(num, 10)
                encoded.append(SymExtra(17, amount-3, 3))
                num -= amount
            while num >= 1:
                encoded.append(SymExtra(0, 0, 0))
                num -= 1
        else:
            encoded.append(SymExtra(value, 0, 0))
            num -= 1
            while num >= 3:
                amount = min(num, 6)
                encoded.append(SymExtra(16, amount-3, 2))
                num -= amount
            while num >= 1:
                encoded.append(SymExtra(value, 0, 0))
                num -= 1
    return encoded

def write_encoded_huff_bits(buf, codes, syms, desc):
    value = 0
    prev = 0
    for code in codes:
        sym = code.symbol
        num = 1
        if sym <= 15:
            buf.push_rev_code(syms[sym], "{} {} bits: {}".format(desc, value, sym))
            prev = sym
        elif sym == 16:
            num = code.extra + 3
            buf.push_rev_code(syms[sym], "{} {}-{} bits: {}".format(desc, value, value+num-1, prev))
        elif sym == 17:
            num = code.extra + 3
            buf.push_rev_code(syms[sym], "{} {}-{} bits: {}".format(desc, value, value+num-1, 0))
        elif sym == 18:
            num = code.extra + 11
            buf.push_rev_code(syms[sym], "{} {}-{} bits: {}".format(desc, value, value+num-1, 0))
        value += num
        if code.bits > 0:
            buf.push(code.extra, code.bits, "{} N={}".format(desc, num))

def prune_matches(matches, offset, opts):
    new_matches = defaultdict(list)
    begin = offset - opts.max_match_distance
    for trigraph,chain in matches.items():
        new_chain = [o for o in chain if o >= begin]
        if new_chain:
            new_matches[trigraph] = new_chain
    return new_matches

def match_block(data, opts=Options()):
    message = []
    matches = defaultdict(list)
    literal = []
    offset = 0
    size = len(data)
    prune_interval = 0
    while offset + 3 <= size:
        trigraph = data[offset:offset+3]
        advance = 1
        match_begin, match_length = 0, 0
        search_steps = 0

        for m in reversed(matches[trigraph]):
            length = 3
            while offset + length < size and length < 258:
                if data[offset + length] != data[m + length]: break
                length += 1
                search_steps += 1
            if length > match_length and m - offset <= 32768:
                match_begin, match_length = m, length
            if search_steps >= opts.search_budget:
                break

        if match_length > 0:
            if literal:
                message.append(Literal(bytes(literal)))
                literal.clear()
            message.append(Match(match_length, offset - match_begin))
            advance = match_length
        else:
            literal.append(data[offset])

        for n in range(advance):
            if offset >= 3:
                trigraph = data[offset - 3:offset]
                matches[trigraph].append(offset - 3)
            offset += 1

        prune_interval += advance
        if prune_interval >= opts.prune_interval:
            matches = prune_matches(matches, offset, opts)
            prune_interval = 0

    while offset < size:
        literal.append(data[offset])
        offset += 1

    if literal:
        message.append(Literal(bytes(literal)))

    return message

def compress_block_uncompressed(buf, data, align, final, opts):
    size = len(data)
    first = True
    begin = 0
    while begin < size or first:
        first = False

        amount = min(size - begin, opts.max_uncompressed_length)
        end = begin + amount
        real_final = final and end == size
        buf.push(real_final, 1, "BFINAL      Final chunk: {}".format(real_final))
        buf.push(0b00, 2,       "BTYPE       Chunk type: Uncompressed")

        buf.push(0, -(buf.pos + align) & 7, "Pad to byte")

        buf.push(amount, 16, "LEN: {}".format(amount))
        buf.push(~amount&0xffff, 16, "NLEN: ~{}".format(amount))
        for byte in data[begin:end]:
            buf.push(byte, 8, "Byte '{}' ({:02x})".format(chr(byte), byte))
        begin = end

def compress_block_static(buf, message, final, opts):
    litlen_bits = [8]*(144-0) + [9]*(256-144) + [7]*(280-256) + [8]*(288-280)
    distance_bits = [5] * 32

    litlen_syms = make_huffman_codes(dict(enumerate(litlen_bits)), 16)
    distance_syms = make_huffman_codes(dict(enumerate(distance_bits)), 16)

    buf.push(final, 1, "BFINAL      Final chunk: {}".format(final))
    buf.push(0b01, 2,  "BTYPE       Chunk type: Static Huffman")

    for m in message:
        m.encode(buf, litlen_syms, distance_syms, opts)

    # End-of-block
    buf.push_rev_code(litlen_syms.get(256, opts.invalid_sym), "End-of-block")

def compress_block_dynamic(buf, message, final, opts):
    litlen_count = [0] * 286
    distance_count = [0] * 30

    # There's always one end-of-block
    litlen_count[256] = 1

    for m in message:
        m.count_codes(litlen_count, distance_count)

    for sym,count in opts.override_litlen_counts.items():
        litlen_count[sym] = count
    for sym,count in opts.override_dist_counts.items():
        distance_count[sym] = count

    litlen_map = { sym: count for sym,count in enumerate(litlen_count) if count > 0 }
    distance_map = { sym: count for sym,count in enumerate(distance_count) if count > 0 }

    litlen_syms = make_huffman(litlen_map, 15)
    distance_syms = make_huffman(distance_map, 15)

    num_litlens = max(itertools.chain((k for k in litlen_map.keys()), (256,))) + 1
    num_distances = max(itertools.chain((k for k in distance_map.keys()), (0,))) + 1

    litlen_bits = [litlen_syms.get(s, null_code).bits for s in range(num_litlens)]
    distance_bits = [distance_syms.get(s, null_code).bits for s in range(num_distances)]

    litlen_bit_codes = encode_huff_bits(litlen_bits)
    distance_bit_codes = encode_huff_bits(distance_bits)

    codelen_count = [0] * 20
    for code in itertools.chain(litlen_bit_codes, distance_bit_codes):
        codelen_count[code.symbol] += 1

    codelen_map = { sym: count for sym,count in enumerate(codelen_count) if count > 0 }
    codelen_syms = make_huffman(codelen_map, 8)

    codelen_permutation = [16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15]

    num_codelens = 0
    for i, p in enumerate(codelen_permutation):
        if codelen_count[p] > 0:
            num_codelens = i + 1
    num_codelens = max(num_codelens, 4)

    buf.push(final, 1, "BFINAL      Final chunk: {}".format(final))
    buf.push(0b10, 2,  "BTYPE       Chunk type: Dynamic Huffman")

    buf.push(num_litlens - 257, 5, "HLIT        Number of Litlen codes: {} (257 + {})".format(num_litlens, num_litlens - 257))
    buf.push(num_distances - 1, 5, "HDIST       Number of Distance codes: {} (1 + {})".format(num_distances, num_distances - 1))
    buf.push(num_codelens - 4, 4,  "HCLEN       Number of Codelen codes: {} (4 + {})".format(num_codelens, num_codelens - 4))

    for p in codelen_permutation[:num_codelens]:
        bits = 0
        if p in codelen_syms:
            bits = codelen_syms[p].bits
        buf.push(bits, 3, "Codelen {} bits: {}".format(p, bits))

    write_encoded_huff_bits(buf, litlen_bit_codes, codelen_syms, "Litlen")
    write_encoded_huff_bits(buf, distance_bit_codes, codelen_syms, "Distance")

    for m in message:
        m.encode(buf, litlen_syms, distance_syms, opts)

    # End-of-block
    buf.push_rev_code(litlen_syms.get(256, opts.invalid_sym), "End-of-block")

def adler32(data):
    a, b = 1, 0
    for d in data:
        a = (a + d) % 65521
        b = (b + a) % 65521
    return b << 16 | a

def compress_message(message, opts=Options(), *args):
    buf = BitBuf()

    # ZLIB CFM byte
    buf.push(8, 4,  "CM=8        Compression method: DEFLATE")
    buf.push(7, 4,  "CINFO=7     Compression info: 32kB window size")

    # ZLIB FLG byte
    buf.push(28, 5, "FCHECK      (CMF*256+FLG) % 31 == 0")
    buf.push(0, 1,  "FDICT=0     Preset dictionary: No")
    buf.push(2, 2,  "FLEVEL=2    Compression level: Default")

    multi_part = False
    multi_messages = []
    multi_opts = []
    if args:
        multi_part = True
        multi_messages = [message]
        multi_opts = [opts]
        args_it = iter(args)
        message = message[:]
        for msg, opt in zip(args_it, args_it):
            message += msg
            multi_messages.append(msg)
            multi_opts.append(opt)

    byte_offset = 0
    part_pos = 0
    num_parts = len(message)
    overflow_part = Literal(b"")
    block_message = []
    block_opts = opts

    message_bytes = b"" if opts.no_decode else decode(message)

    last_part = False
    multi_index = 0
    while not last_part:
        if multi_part:
            block_message = multi_messages[multi_index]
            block_opts = multi_opts[multi_index]
            size = sum(m.length for m in block_message)
            block_index = 0

            multi_index += 1
            last_part = multi_index == len(multi_messages)
        else:
            block_message.clear()

            part, overflow_part = overflow_part.split(opts.block_size)
            if part.length > 0:
                block_message.append(part)
            size = part.length

            # Append parts until desired block size is reached
            if size < opts.block_size:
                while part_pos < num_parts:
                    part = message[part_pos]
                    part_pos += 1
                    if size + part.length >= opts.block_size:
                        last_part, overflow_part = part.split(opts.block_size - size)
                        if last_part.length > 0:
                            block_message.append(last_part)
                        size += last_part.length
                        break
                    else:
                        block_message.append(part)
                        size += part.length

            last_part = part_pos >= num_parts and overflow_part.length == 0

        # Compress the block
        best_buf = None
        block_index = 0
        for block_type in range(3):
            if block_index < len(block_opts.force_block_types):
                if block_type != block_opts.force_block_types[block_index]:
                    continue

            block_buf = BitBuf()

            if block_type == 0:
                compress_block_uncompressed(block_buf, message_bytes[byte_offset:byte_offset + size], buf.pos, last_part, block_opts)
            elif block_type == 1:
                compress_block_static(block_buf, block_message, last_part, block_opts)
            elif block_type == 2:
                compress_block_dynamic(block_buf, block_message, last_part, block_opts)

            if not best_buf or block_buf.pos < best_buf.pos:
                best_buf = block_buf

        buf.append(best_buf)
        byte_offset += size
        block_index += 1

    buf.push(0, -buf.pos & 7, "Pad to byte")

    adler_hash = adler32(message_bytes)

    buf.push((adler_hash >> 24) & 0xff, 8, "Adler[24:32]")
    buf.push((adler_hash >> 16) & 0xff, 8, "Adler[16:24]")
    buf.push((adler_hash >>  8) & 0xff, 8, "Adler[8:16]")
    buf.push((adler_hash >>  0) & 0xff, 8, "Adler[0:8]")

    return buf

def deflate(data, opts=Options()):
    message = match_block(data, opts)
    encoded = compress_message(message, opts)
    return encoded

def print_huffman(tree):
    width = max(len(str(s)) for s in tree.keys())
    for sym,code in tree.items():
        print("".format(sym, width, code.code, code.bits))

def print_buf(buf):
    for d in buf.desc:
        val = " {0:0{1}b}".format(d.value, d.bits)
        if len(val) > 10:
            val = "0x{0:x}".format(d.value)
        desc = d.desc
        patched_value = (buf.data >> d.offset) & ((1 << d.bits) - 1)
        spacer = "|"
        if patched_value != d.value:
            desc += "  >>> Patched to: {0:0{1}b} ({0})".format(patched_value, d.bits)
            spacer = ">"
        print("{0:>4} {0:>4x} {5}{1:>2} {5} {2:>10} {5} {3:>4} {5} {4}".format(d.offset, d.bits, val, d.value, desc, spacer))

def print_bytes(data):
    print(''.join('\\x%02x' % b for b in data))
