#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <cctype>

bool g_verbose = false;

static void ufbxt_assert_fail_imp(const char *func, const char *file, size_t line, const char *msg)
{
	fprintf(stderr, "%s:%zu: %s(%s) failed\n", file, line, func, msg);
	exit(2);
}

#define ufbxt_assert_fail(file, line, msg) ufbxt_assert_fail_imp("ufbxt_assert_fail", file, line, msg)
#define ufbxt_assert(m_cond) do { if (!(m_cond)) ufbxt_assert_fail_imp("ufbxt_assert", __FILE__, __LINE__, #m_cond); } while (0)

#include <vector>
#include <memory>
#include <string>

#include "../ufbx.h"
#include "check_scene.h"

#define arraycount(arr) ((sizeof(arr))/(sizeof(*(arr))))

const char *interesting_tokens[] = {
    "",
    " ",
    "/",
    "0",
    "1.0",
    ".0",
    ".",
    "1e64",
    "-1",
    "-1000",
    "1000",
    "#",
    "-",
    "-x",
    "\n",
    "\\",
    "\\\n",
};

// No need to duplicate `interesting_tokens[]`
const char *interesting_leading_tokens[] = {
    "v",
    "vn",
    "vt",
    "f",
    "l",
    "p",
    "g",
    "o",
    "usemtl",
    "newmtl",
    "mtllib",
    "Kd",
    "map_Kd",
};

const char *interesting_lines[] = {
    "g Objfuzz",
    "usemtl Objfuzz",
    "newmtl Objfuzz",
    "mttlib objfuzz.mtl",
    "v 1 2 3",
    "vt 1 2",
    "vn 1 2 3",
    "f 1 2 3",
    "f 1/1 2/2 3/3",
    "f 1//1 2//2 3//3",
    "f 1/1/1 2/2/2 3/3/3",
    "map_Kd base.png",
};


bool mutate_insert_token(std::vector<std::string> &tokens, uint32_t token, uint32_t index)
{
    if (index >= arraycount(interesting_tokens)) return false;
    tokens.insert(tokens.begin() + token, interesting_tokens[index]);
    return true;
}

bool mutate_replace_token(std::vector<std::string> &tokens, uint32_t token, uint32_t index)
{
    if (index >= arraycount(interesting_tokens)) return false;
    tokens[token] = interesting_tokens[index];
    return true;
}

bool mutate_duplicate_token(std::vector<std::string> &tokens, uint32_t token, uint32_t index)
{
    if (index > 0) return false;
    std::string tok = tokens[token];
    tokens.insert(tokens.begin() + token, tok);
    return true;
}

bool mutate_remove_token(std::vector<std::string> &tokens, uint32_t token, uint32_t index)
{
    if (index > 0) return false;
    tokens.erase(tokens.begin() + token);
    return true;
}

bool mutate_replace_first_token(std::vector<std::string> &tokens, uint32_t token, uint32_t index)
{
    if (token > 0) return false;
    if (index >= arraycount(interesting_leading_tokens)) return false;
    tokens[0] = interesting_leading_tokens[index];
    return true;
}

bool mutate_insert_first_token(std::vector<std::string> &tokens, uint32_t token, uint32_t index)
{
    if (token > 0) return false;
    if (index >= arraycount(interesting_leading_tokens)) return false;
    tokens.insert(tokens.begin(), interesting_leading_tokens[index]);
    return true;
}

bool mutate_remove_line(std::vector<std::string> &tokens, uint32_t token, uint32_t index)
{
    if (token > 0 || index > 0) return false;
    tokens.clear();
    return true;
}

bool mutate_insert_line(std::vector<std::string> &tokens, uint32_t token, uint32_t index)
{
    if (token > 0) return false;
    if (index >= arraycount(interesting_lines)) return false;
    tokens.push_back(interesting_lines[index]);
    return true;
}

bool mutate_replace_line(std::vector<std::string> &tokens, uint32_t token, uint32_t index)
{
    if (token > 0) return false;
    if (index >= arraycount(interesting_lines)) return false;
    tokens.clear();
    tokens.push_back(interesting_lines[index]);
    return true;
}

bool mutate_duplicate_line(std::vector<std::string> &tokens, uint32_t token, uint32_t index)
{
    if (token > 0 || index > 0) return false;
    auto copy = tokens;
    tokens.insert(tokens.end(), copy.begin(), copy.end());
    return true;
}

enum class token_category {
    initial,
    slash,
    comment,
    whitespace,
    newline,
    other,
    end,
};

token_category categorize_char(char c)
{
    switch (c) {
    case '/':
        return token_category::slash;
    case '#':
        return token_category::comment;
    case ' ':
    case '\t':
        return token_category::whitespace;
    case '\r':
    case '\n':
        return token_category::newline;
    default:
        return token_category::other;
    }
}

std::vector<std::string> tokenize_line(const std::string &line)
{
    std::vector<std::string> result;

    size_t len = line.length();

    size_t prev_begin = 0;
    token_category prev_cat = token_category::initial;

    for (size_t i = 0; i <= len; i++) {
        token_category cat = i < len ? categorize_char(line[i]) : token_category::end;
        if (cat != prev_cat) {
            if (prev_begin < i) {
                result.push_back(line.substr(prev_begin, i - prev_begin));
            }

            prev_begin = i;
            prev_cat = cat;
        }
    }

    return result;
}

using mutate_fn = bool(std::vector<std::string> &tokens, uint32_t token, uint32_t index);

struct mutator
{
	const char *name;
	mutate_fn *fn;
};

static const mutator mutators[] = {
    { "insert token", &mutate_insert_token },
    { "replace token", &mutate_replace_token },
    { "duplicate token", &mutate_duplicate_token },
    { "remove token", &mutate_remove_token },
    { "replace first token", &mutate_replace_first_token },
    { "insert first token", &mutate_insert_first_token },
    { "remove line", &mutate_remove_line },
    { "insert line", &mutate_insert_line },
    { "replace line", &mutate_replace_line },
    { "duplicate line", &mutate_duplicate_line },
};

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

std::vector<std::string> split_lines(const char *data, size_t length)
{
    std::vector<std::string> lines;
    size_t line_begin = 0;
    for (size_t i = 0; i <= length; i++) {
        char c = i < length ? data[i] : '\0';
        if (c == '\n' || c == '\0') {
            size_t inclusive = c == '\n' ? 1 : 0;
            lines.emplace_back(data + line_begin, i - line_begin + inclusive);
            line_begin = i + 1;
        }
    }
    return lines;
}

std::string concatenate(const std::string *parts, size_t count)
{
    std::string result;
    for (size_t i = 0; i < count; i++) {
        result += parts[i];
    }
    return result;
}

void process_file(const char *input_file, ufbx_file_format file_format)
{
	std::vector<char> data = read_file(input_file);
    std::vector<std::string> lines = split_lines(data.data(), data.size());
    int current_step = 0;

    // Add dummy line to end for mutators
    lines.emplace_back();

    for (uint32_t line = 0; line < lines.size(); line++) {
        std::vector<std::string> tokens = tokenize_line(lines[line]);

		// Add dummy token to end for mutators
        tokens.emplace_back();

        for (uint32_t mut = 0; mut < arraycount(mutators); mut++) {
            bool done = false;
            for (uint32_t token = 0; !done && token < tokens.size(); token++) {
                for (uint32_t index = 0; index < tokens.size(); index++) {
                    std::vector<std::string> copy = tokens;
                    bool ok = mutators[mut].fn(copy, token, index);
                    if (!ok) {
                        done = index == 0;
                        break;
                    }

                    std::string parts[] = {
                        concatenate(lines.data(), line),
                        concatenate(copy.data(), copy.size()),
                        concatenate(lines.data() + line + 1, lines.size() - (line + 1)),
                    };
                    std::string file = concatenate(parts, arraycount(parts));

                    if (g_verbose) {
                        printf("%d: line %u token %u: %s %u: ", current_step, line, token, mutators[mut].name, index);
                    }

                    ufbx_load_opts opts = { };
                    opts.file_format = file_format;

                    ufbx_error error;
                    ufbx_scene *scene = ufbx_load_memory(file.data(), file.length(), &opts, &error);
                    if (scene) {
                        if (g_verbose) {
                            printf("OK!\n");
                        }
                        ufbxt_check_scene(scene);
                        ufbx_free_scene(scene);
                    } else {
                        if (g_verbose) {
                            printf("%s\n", error.description.data);
                        }
                    }

                    current_step++;
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
	const char *input_file = NULL;
    ufbx_file_format file_format = UFBX_FILE_FORMAT_OBJ;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			g_verbose = true;
		} else if (!strcmp(argv[i], "--mtl")) {
			file_format = UFBX_FILE_FORMAT_MTL;
		} else {
			input_file = argv[i];
		}
	}

	if (input_file) {
		process_file(input_file, file_format);
	} else {
		fprintf(stderr, "Usage: objfuzz <file.obj>\n");
	}

	return 0;
}
