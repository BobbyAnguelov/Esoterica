#define _CRT_SECURE_NO_WARNINGS
#include "picort.h"

bool g_verbose;

static void parse_args(Opts &opts, int argc, char **argv, bool ignore_input)
{
	for (int argi = 1; argi < argc; ) {
		const char *arg = argv[argi++];
		if (arg[0] == '-') {
			arg++;
			if (arg[0] == '-') arg++;
			for (OptBase &opt : opts) {
				if (!ignore_input && !strcmp(opt.name, "input")) continue;
				if (!strcmp(opt.name, arg) || (opt.alias && !strcmp(opt.alias, arg))) {
					if ((uint32_t)(argc - argi) >= opt.num_args) {
						opt.parse((const char**)argv + argi);
						opt.defined = true;
						opt.from_arg = true;
						argi += opt.num_args;
						break;
					}
				}
			}
		} else {
			if (!ignore_input) {
				opts.input.value = arg;
				opts.input.defined = true;
				opts.input.from_arg = true;
			}
		}
	}
}

static size_t parse_opts_line(const char **tokens, char *line, size_t max_tokens)
{
	size_t num_tokens = 0;
	char *src = line, *dst = line;
	const char *token = dst;
	while (num_tokens < max_tokens) {
		while (*src == ' ' || *src == '\r' || *src == '\t' || *src == '\n') {
			src++;
		}
		if (*src == '\0' || *src == '#') break;
		if (*src == '"') {
			src++;
			while (*src != '\0' && *src != '"') {
				if (*src == '\\') src++;
				*dst++ = *src++;
			}
		} else {
			while (*src != '\0' && *src != ' ' && *src != '\t' && *src != '\r' && *src != '\n') {
				*dst++ = *src++;
			}
		}
		if (*src != '\0') src++;
		*dst++ = '\0';
		tokens[num_tokens++] = token;
		token = dst;
	}
	return num_tokens;
}

static void parse_opts_file(Opts &opts, const char *filename)
{
	FILE *f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "Failed to open: %s\n", filename);
		exit(1);
	}

	{
		size_t len = strlen(filename);
		while (len > 0 && filename[len - 1] != '\\' && filename[len - 1] != '/') {
			len--;
		}
		opts.base_path.value = std::string{ filename, len };
		opts.base_path.defined = true;
	}

	char line[1024];
	const char *tokens[16];
	while (fgets(line, sizeof(line), f)) {
		size_t num_tokens = parse_opts_line(tokens, line, 16);
		if (num_tokens < 1) continue;
		for (OptBase &opt : opts) {
			if (!strcmp(opt.name, tokens[0]) && num_tokens - 1 >= opt.num_args) {
				opt.parse(tokens + 1);
				opt.defined = true;
				break;
			}
		}
	}

	fclose(f);
}

void compare_images(Opts &opts, const char *path_a, const char *path_b)
{
	const char *paths[] = { path_a, path_b };
	Image img[2];

	for (uint32_t i = 0; i < 2; i++) {
		const char *path = paths[i];
		ImageResult result = load_png(path);
		if (result.error) {
			fprintf(stderr, "Failed to load %s: %s\n", path, result.error);
			exit(1);
		}
		img[i] = std::move(result.image);
	}

	if (img[0].width != img[1].width || img[0].height != img[1].height) {
		fprintf(stderr, "Resolution mismatch: %ux%u vs %ux%u\n",
			img[0].width, img[0].height, img[1].width, img[1].height);
		exit(1);
	}

	double error = 0.0;
	for (uint32_t y = 0; y < img[0].height; y++) {
		for (uint32_t x = 0; x < img[0].width; x++) {
			Pixel16 a = img[0].pixels[y * img[0].width + x];
			Pixel16 b = img[1].pixels[y * img[1].width + x];
			for (uint32_t c = 0; c < 4; c++) {
				uint16_t ca = (&a.r)[c], cb = (&b.r)[c];
				double va = (double)ca * (1.0/65535.0), vb = (double)cb * (1.0/65535.0);
				error += (va - vb) * (va - vb);
			}
		}
	}
	error /= (double)(img[0].width * img[0].height);

	printf("Difference (MSE): %.4f\n", error);
	if (error > opts.error_threshold.value) {
		printf("ERROR: Over threshold of %.4f\n", opts.error_threshold.value);
		exit(1);
	}
}

Opts setup_opts(int argc, char **argv)
{
	Opts opts;
	parse_args(opts, argc, argv, false);

	if (opts.help.value) {
		fprintf(stderr, "Usage: picort input.fbx <opts> (--help)\n");
		for (OptBase &opt : opts) {
			char name[64];
			if (opt.alias) {
				snprintf(name, sizeof(name), "--%s (-%s)", opt.name, opt.alias);
			} else {
				snprintf(name, sizeof(name), "--%s", opt.name);
			}
			fprintf(stderr, "  %-20s %s\n", name, opt.desc);
		}
		return { };
	}

	if (opts.verbose.value) {
		g_verbose = true;
	}

	if (ends_with(opts.input.value, ".txt")) {
		std::string path = std::move(opts.input.value);
		opts = Opts{};
		parse_opts_file(opts, path.c_str());
		parse_args(opts, argc, argv, true);
	}

	if (opts.verbose.defined) {
		char buf[512];
		for (OptBase &opt : opts) {
			if (opt.defined) {
				opt.print(buf, sizeof(buf));
				if (opt.from_arg) {
					printf("%s: %s (--)\n", opt.name, buf);
				} else {
					printf("%s: %s\n", opt.name, buf);
				}
			}
		}
	}

	if (opts.num_frames.defined) {
		if (!opts.time.defined) {
			opts.frame.defined = true;
		}
	}

	return opts;
}

std::string get_path(const Opts &opts, const Opt<std::string> &opt)
{
	if (opts.base_path.defined && !opt.from_arg) {
		return opts.base_path.value + opt.value;
	} else {
		return opt.value;
	}
}
