
# Generate lookup table for `base + read(bits)` Deflate operands
def deflate_lookup_table(pairs):
    for (base, bits, flags) in pairs:
        assert bits < (1 << 4)
        assert (flags & 0xe0) == flags
        assert base < (1 << 16)
        yield bits | flags | base << 16

def format_table(data, cols):
    data = list(data)
    for base in range(0, len(data), cols):
        yield ''.join('0x{:08x}, '.format(x) for x in data[base:base+cols])

# Deflate RFC 1951 3.2.5. tables
length_operands = [
    (0,0,0x20),(3,0,0x40),(4,0,0x40),(5,0,0x40),(6,0,0x40),(7,0,0x40),(8,0,0x40),(9,0,0x40),(10,0,0x40),(11,1,0x40),
    (13,1,0x40),(15,1,0x40),(17,1,0x40),(19,2,0x40),(23,2,0x40),(27,2,0x40),(31,2,0x40),(35,3,0x40),
    (43,3,0x40),(51,3,0x40),(59,3,0x40),(67,4,0x40),(83,4,0x40),(99,4,0x40),(115,4,0x40),(131,5,0x40),
    (163,5,0x40),(195,5,0x40),(227,5,0x40),(258,0,0x40),
    (1,0,0x20),(1,0,0x20),
]
dist_operands = [
    (1,0,0),(2,0,0),(3,0,0),(4,0,0),(5,1,0),(7,1,0),(9,2,0),(13,2,0),(17,3,0),
    (25,3,0),(33,4,0),(49,4,0),(65,5,0),(97,5,0),(129,6,0),(193,6,0),(257,7,0),
    (385,7,0),(513,8,0),(769,8,0),(1025,9,0),(1537,9,0),(2049,10,0),(3073,10,0),
    (4097,11,0),(6145,11,0),(8193,12,0),(12289,12,0),(16385,13,0),(24577,13,0),
    (1,0,0x20),(1,0,0x20),
]

print('static const uint32_t ufbxi_deflate_length_lut[] = {')
table = deflate_lookup_table(length_operands)
print('\n'.join('\t' + t for t in format_table(table, 8)))
print('};')

print('static const uint32_t ufbxi_deflate_dist_lut[] = {')
table = deflate_lookup_table(dist_operands)
print('\n'.join('\t' + t for t in format_table(table, 8)))
print('};')
