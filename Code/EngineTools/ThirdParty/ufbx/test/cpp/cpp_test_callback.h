
UFBXT_CPP_TEST(test_callback_progress)
{
	size_t num_calls = 0;
	auto progress_cb = [&](const ufbx_progress*){
		++num_calls;
		return UFBX_PROGRESS_CONTINUE;
	};

	ufbx_load_opts opts = { };
	opts.progress_cb = &progress_cb;
	opts.progress_interval_hint = 1;

	load_scene("maya_cube_7500_ascii.fbx", opts);
	ufbxt_assert(num_calls >= 10000);
}
