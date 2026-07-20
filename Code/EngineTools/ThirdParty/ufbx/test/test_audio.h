#undef UFBXT_TEST_GROUP
#define UFBXT_TEST_GROUP "audio"

UFBXT_FILE_TEST(maya_audio)
#if UFBXT_IMPL
{
	ufbx_audio_layer *layer = ufbx_as_audio_layer(ufbx_find_element(scene, UFBX_ELEMENT_AUDIO_LAYER, "audio_track1"));
	ufbxt_assert(layer);

	ufbx_audio_clip *clip = ufbx_as_audio_clip(ufbx_find_element(scene, UFBX_ELEMENT_AUDIO_CLIP, "plonk"));
	ufbxt_assert(clip);

	ufbxt_assert(layer->clips.count == 1);
	ufbxt_assert(layer->clips.data[0] == clip);

	ufbxt_assert(!strcmp(clip->relative_filename.data, "audio\\plonk.wav"));
	ufbxt_assert(!strcmp(clip->absolute_filename.data, "D:/Dev/clean/ufbx/data/audio/plonk.wav"));

	ufbxt_check_blob_content(clip->content, "audio/plonk.wav");
}
#endif

UFBXT_FILE_TEST(maya_audio_clips)
#if UFBXT_IMPL
{
	ufbx_audio_layer *layer1 = ufbx_as_audio_layer(ufbx_find_element(scene, UFBX_ELEMENT_AUDIO_LAYER, "audio_track1"));
	ufbxt_assert(layer1);
	ufbx_audio_layer *layer2 = ufbx_as_audio_layer(ufbx_find_element(scene, UFBX_ELEMENT_AUDIO_LAYER, "audio_track2"));
	ufbxt_assert(layer2);

	ufbx_audio_clip *plonk = ufbx_as_audio_clip(ufbx_find_element(scene, UFBX_ELEMENT_AUDIO_CLIP, "plonk"));
	ufbxt_assert(plonk);
	ufbx_audio_clip *plonk1 = ufbx_as_audio_clip(ufbx_find_element(scene, UFBX_ELEMENT_AUDIO_CLIP, "plonk1"));
	ufbxt_assert(plonk1);
	ufbx_audio_clip *plonk_bitcrush = ufbx_as_audio_clip(ufbx_find_element(scene, UFBX_ELEMENT_AUDIO_CLIP, "plonk_bitcrush"));
	ufbxt_assert(plonk_bitcrush);

	ufbxt_assert(layer1->clips.count == 2);
	ufbxt_assert(layer1->clips.data[0] == plonk1);
	ufbxt_assert(layer1->clips.data[1] == plonk);

	ufbxt_assert(layer2->clips.count == 1);
	ufbxt_assert(layer2->clips.data[0] == plonk_bitcrush);

	ufbxt_assert(!strcmp(plonk->relative_filename.data, "audio\\plonk.wav"));
	ufbxt_assert(!strcmp(plonk->absolute_filename.data, "D:/Dev/clean/ufbx/data/audio/plonk.wav"));
	ufbxt_assert(!strcmp(plonk1->relative_filename.data, "audio\\plonk.wav"));
	ufbxt_assert(!strcmp(plonk1->absolute_filename.data, "D:/Dev/clean/ufbx/data/audio/plonk.wav"));
	ufbxt_assert(!strcmp(plonk_bitcrush->relative_filename.data, "audio\\plonk_bitcrush.wav"));
	ufbxt_assert(!strcmp(plonk_bitcrush->absolute_filename.data, "D:/Dev/clean/ufbx/data/audio/plonk_bitcrush.wav"));

	ufbxt_check_blob_content(plonk->content, "audio/plonk.wav");
	ufbxt_check_blob_content(plonk1->content, "audio/plonk.wav");
	ufbxt_check_blob_content(plonk_bitcrush->content, "audio/plonk_bitcrush.wav");
}
#endif


