#ifndef SEMFUZZ_STRING_TABLE_H
#define SEMFUZZ_STRING_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#define STRING_TABLE_SIZE 1027
extern const char *string_table[STRING_TABLE_SIZE];

#define UNKNOWN_STRING_TABLE_SIZE 3923
extern const char *unknown_string_table[UNKNOWN_STRING_TABLE_SIZE];

#ifdef __cplusplus
}
#endif

#endif
