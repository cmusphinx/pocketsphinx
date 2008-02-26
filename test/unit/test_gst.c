#include <gst/gst.h>

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
	GstElement *pipeline;
	GstElement *src, *resamp, *filter, *sink;
	GstCaps *caps;
	GstBus *bus;

	gst_init(&argc, &argv);
	loop = g_main_loop_new(NULL, FALSE);

	pipeline = gst_pipeline_new("test_gst");
	src = gst_element_factory_make("filesrc", "file-source");
	g_object_set(G_OBJECT(src), "location", DATADIR "/goforward.raw", NULL);
	caps = gst_caps_from_string("audio/x-raw-int,channels=1,endianness=1234,width=16,depth=16,rate=16000,signed=true");
	if (caps == NULL) {
		g_print("Failed to create caps!\n");
		return 1;
	}
	resamp = gst_element_factory_make("audioresample", "resampler");
	filter = gst_element_factory_make("pocketsphinx", "asr");
	sink = gst_element_factory_make("fakesink", "sink");
	gst_bin_add_many(GST_BIN(pipeline),
			 src, resamp, filter, sink, NULL);
	gst_element_link_filtered(src, resamp, caps);
	gst_element_link(resamp, filter);
	gst_element_link(filter, sink);

	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, bus_call, loop);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	g_main_loop_run(loop);

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(pipeline));

	return 0;
}
