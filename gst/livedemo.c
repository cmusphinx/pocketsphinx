#include <gst/gst.h>
#include <glib.h>

static gboolean
bus_call(GstBus * bus, GstMessage * msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *) data;

    switch (GST_MESSAGE_TYPE(msg)) {

    case GST_MESSAGE_EOS:
        g_print("End of stream\n");
        g_main_loop_quit(loop);
        break;

    case GST_MESSAGE_ERROR:{
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            g_printerr("Error: %s\n", error->message);
            g_error_free(error);

            g_main_loop_quit(loop);
            break;
        }
    default:
        break;
    }
    
    const GstStructure *st = gst_message_get_structure(msg);
    if (st && strcmp(gst_structure_get_name(st), "pocketsphinx") == 0) {
	if (g_value_get_boolean(gst_structure_get_value(st, "final")))
    	    g_print("Got result %s\n", g_value_get_string(gst_structure_get_value(st, "hypothesis")));
    }

    return TRUE;
}


int
main(int argc, char *argv[])
{
    GMainLoop *loop;

    GstElement *pipeline, *source, *decoder, *sink;
    GstBus *bus;
    guint bus_watch_id;

    /* Initialisation */
    gst_init(&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE);

    /* Check input arguments */
    if (argc != 2) {
        g_printerr("Usage: %s <file.raw>\n", argv[0]);
        return -1;
    }

    /* Create gstreamer elements */
    pipeline = gst_pipeline_new("pipeline");
    source = gst_element_factory_make("filesrc", "file-source");
    decoder = gst_element_factory_make("pocketsphinx", "asr");
    sink = gst_element_factory_make("fakesink", "output");

    if (!pipeline || !source || !decoder || !sink) {
        g_printerr("One element could not be created. Exiting.\n");
        return -1;
    }

    /* Set up the pipeline */
    /* we set the input filename to the source element */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);

    g_object_set(G_OBJECT(decoder), "lmctl", "test.lmctl", NULL);
    g_object_set(G_OBJECT(decoder), "lmname", "tidigits", NULL);

    /* we add a message handler */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    /* we add all elements into the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, decoder, sink, NULL);

    /* we link the elements together */
    gst_element_link_many(source, decoder, sink, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Iterate */
    g_print("Running...\n");
    g_main_loop_run(loop);

    /* Out of the main loop, clean up nicely */
    g_print("Returned, stopping playback\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);

    g_print("Deleting pipeline\n");
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);

    return 0;
}
