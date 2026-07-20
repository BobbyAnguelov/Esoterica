
categories = [
    " \r\n\t",         # UFBXI_XML_CTYPE_WHITESPACE
    "\'",              # UFBXI_XML_CTYPE_SINGLE_QUOTE
    "\"",              # UFBXI_XML_CTYPE_DOUBLE_QUOTE
    " \r\n\t?/>\'\"=", # UFBXI_XML_CTYPE_NAME_END
    "<",               # UFBXI_XML_CTYPE_TAG_START
    "\0",              # UFBXI_XML_CTYPE_END_OF_FILE
]

def generate_bits(categories):
    for ix in range(256):
        ch = chr(ix)
        bits = 0
        for bit, cat in enumerate(categories):
            if ch in cat:
                bits |= 1 << bit
        yield bits

bits = list(generate_bits(categories))

# Truncate down to what's needed
num_bits = 64
assert all(b == 0 for b in bits[num_bits:])
bits = bits[:num_bits]

chunk = 32
for base in range(0, len(bits), chunk):
    print("".join("{},".format(b) for b in bits[base:base+chunk]))
