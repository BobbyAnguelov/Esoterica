#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdint.h>
#include <string.h>

namespace fbxdom {

struct node;
using node_ptr = std::shared_ptr<node>;

struct array_info
{
	uint32_t length = 0;
	uint32_t encoding = 0;
	uint32_t compressed_length = 0;
};

struct value
{
	char type = 0;
	array_info data_array;
	int64_t data_int = 0;
	double data_float = 0.0;
	std::vector<char> data;
};

struct node
{
	std::vector<char> name;
	std::vector<value> values;
	std::vector<node_ptr> children;

	node_ptr copy() const;
};

node_ptr parse(const void *data, size_t size);
size_t dump(void *dst, size_t size, node_ptr root);

}
