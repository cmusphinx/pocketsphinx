#include <gst/gst.h>
#include <string.h>

#include "test_macros.h"

static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = (GMainLoop *)data;

	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_EOS:
		g_print("End-of-stream\n");
		g_main_loop_quit(loop);
		break;
	case GST_MESSAGE_ERROR: {
		gchar *debug;
		GError *err;

		gst_message_parse_error(msg, &err, &debug);
		g_free(debug);
		g_print("Error: %s\n", err->message);
		g_error_free(err);
		g_main_loop_quit(loop);
		break;
	}
	default:
		break;
	}

	return TRUE;
}

int
main(int argc, char *argv[])
{
	GMainLoop *loop;
	GError *err;
	GstElement *pipeline;
	GstElement *src, *resamp, *filter, *vader, *sink;
	GstCaps *caps;
	GstBus *bus;
	FILE *fh;
	char line[256];

	gst_init(&argc, &argv);
	err = NULL;
	gst_plugin_load_file("../../src/gst-plugin/.libs/libgstpocketsphinx.so", &err);
	if (err) {
		g_print("Failed to load plugin: %s\n", err->message);
		return 1;
	}
	loop = g_main_loop_new(NULL, FALSE);

	pipeline = gst_pipeline_new("test_gst");
	src = gst_element_factory_make("filesrc", "file-source");
	g_object_set(G_OBJECT(src), "location", DATADIR "/goforward.raw", NULL);
	caps = gst_caps_from_string("audio/x-raw-int,channels=1,endianness=1234,width=16,depth=16,rate=16000,signed=(bool)true");
	if (caps == NULL) {
		g_print("Failed to create caps!\n");
		return 1;
	}
	resamp = gst_element_factory_make("audioresample", "resampler");
	vader = gst_element_factory_make("vader", "vad");
	g_object_set(G_OBJECT(vader), "auto_threshold", TRUE, NULL);
	filter = gst_element_factory_make("pocketsphinx", "asr");
	g_object_set(G_OBJECT(filter), "hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k", NULL);
	g_object_set(G_OBJECT(filter), "lm", MODELDIR "/lm/en/turtle.DMP", NULL);
	g_object_set(G_OBJECT(filter), "dict", MODELDIR "/lm/en/turtle.dic", NULL);
	g_object_set(G_OBJECT(filter), "latdir", ".", NULL);
	sink = gst_element_factory_make("filesink", "sink");
	g_object_set(G_OBJECT(sink), "location", "test_gst.out", NULL);
	gst_bin_add_many(GST_BIN(pipeline),
			 src, resamp, vader, filter, sink, NULL);
	gst_element_link_filtered(src, resamp, caps);
	gst_element_link(resamp, vader);
	gst_element_link(vader, filter);
	gst_element_link(filter, sink);

	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, bus_call, loop);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	g_main_loop_run(loop);

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(pipeline));

	TEST_ASSERT(fh = fopen("test_gst.out", "r"));
	TEST_ASSERT(fgets(line, sizeof(line), fh));
	TEST_EQUAL(0, strcmp(line, "go forward ten meters\n"));

	fclose(fh);

	return 0;
}
