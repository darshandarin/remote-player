#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *filesrc, *decodebin, *id3demux;
    GstBus *bus;
    GstMessage *msg;
    GMainLoop *loop;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    pipeline = gst_pipeline_new("audio-player");
    filesrc = gst_element_factory_make("filesrc", "file-source");
    decodebin = gst_element_factory_make("decodebin", "decoder");
    id3demux = gst_element_factory_make("id3demux", "id3-demux");

    /* Set the input file location */
    g_object_set(G_OBJECT(filesrc), "uri","file:///home/ee212829/video1.mp4", NULL);

    /* Add the elements to the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), filesrc, decodebin, id3demux, NULL);

    /* Link the elements */
    gst_element_link(filesrc, decodebin);
    gst_element_link(decodebin, id3demux);

    /* Set the bus to watch for messages */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
   // gst_bus_add_watch(bus, loop);
    gst_object_unref(bus);

    /* Start the pipeline */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Run the main loop */
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    /* Clean up */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    return 0;
}
