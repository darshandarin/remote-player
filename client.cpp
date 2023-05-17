#include<string>
#include<vector>
#include <gst/gst.h>
#include<iostream>

typedef struct _CustomData
{
	GstElement *pipeline, *udpsrc, *depay, *decoder, *sink,*queue;
	GstElement *udpsrc1,*audiodepay,*adecoder,*udpsink1,*queue1;
	GstElement *tpipeline, *tudpsrc, *tdepay,*tdecoder,*vconvert, *isink;
} CustomData;

/* main function */
int main(int argc, char *argv[]) {
	GstCaps *caps;
	GstCaps *caps1;
	GstCaps *caps2;
	GstBus *bus;
	GstMessage *msg,*msg1;
	GstStateChangeReturn ret,ret1;
	gboolean terminate = false;
	gst_init(&argc, &argv);
	CustomData data;

	/* creating elements for video */
	data.pipeline = gst_pipeline_new("mypipeline");
	data.udpsrc = gst_element_factory_make("udpsrc", "network-source");

	/* setting port number for video */
	g_object_set(G_OBJECT(data.udpsrc), "port", 5678, NULL);
	caps = gst_caps_new_simple("application/x-rtp", "encoding-name", G_TYPE_STRING, "H264", "payload", G_TYPE_INT, 96, NULL);
	g_object_set(G_OBJECT(data.udpsrc), "caps", caps, NULL);
	data.queue=gst_element_factory_make("queue","queue");
	data.depay = gst_element_factory_make("rtph264depay", "depayloader");
	data.decoder = gst_element_factory_make("avdec_h264", "decoder");
	data.sink = gst_element_factory_make("autovideosink", "video-sink");

	/* creating elements for audio */
	data.udpsrc1 = gst_element_factory_make("udpsrc", "udpsrc1");

	/* setting port number for audio */
	g_object_set(G_OBJECT(data.udpsrc1), "port", 5679, NULL);
	caps1 = gst_caps_new_simple("application/x-rtp", "encoding-name", G_TYPE_STRING, "OPUS", "payload", G_TYPE_INT, 96, NULL);
	g_object_set(G_OBJECT(data.udpsrc1), "caps", caps1, NULL);
	data.queue1=gst_element_factory_make("queue","queue1");
	data.audiodepay = gst_element_factory_make("rtpopusdepay", "audiodepay");
	data.adecoder = gst_element_factory_make("opusdec", "adecoder");
	data.udpsink1 = gst_element_factory_make("autoaudiosink", "udpsink1");

	/* creating elements for thumbnail */
	data.tpipeline = gst_pipeline_new("tpipeline");
	data.tudpsrc = gst_element_factory_make("udpsrc", "udp");

	/* setting port number for thumbnail */
	g_object_set(G_OBJECT(data.tudpsrc), "port", 5555, NULL);
	caps2 = gst_caps_new_simple("application/x-rtp", "encoding-name", G_TYPE_STRING, "H264", NULL);
	g_object_set(G_OBJECT(data.tudpsrc), "caps", caps2, NULL);
	data.tdepay = gst_element_factory_make("rtpjpegdepay", "tdepayloader");
	data.tdecoder = gst_element_factory_make("jpegdec", "tdecoder");
	data.vconvert=gst_element_factory_make("videoconvert", "vconvert");
	data.isink = gst_element_factory_make("ximagesink", "isink");

	/* adding elements to the pipeline */
	gst_bin_add_many(GST_BIN(data.pipeline), data.udpsrc, data.depay,data.queue, data.decoder, data.sink,data.udpsrc1,data.audiodepay,data.adecoder,data.udpsink1,data.queue1, NULL);
	gst_bin_add_many(GST_BIN(data.tpipeline), data.tudpsrc, data.tdepay, data.tdecoder,data.vconvert,data.isink,NULL);

	/* linking elements to display video */
	gst_element_link_many(data.udpsrc,data.queue, data.depay,NULL);
	gst_element_link(data.depay, data.decoder);
	gst_element_link(data.decoder, data.sink);

	/* linking elements to display audio */
	gst_element_link_many(data.udpsrc1,data.queue1, data.audiodepay,NULL);
	gst_element_link(data.audiodepay, data.adecoder);
	gst_element_link(data.adecoder, data.udpsink1);

	/* linking elements to display thumbnail */
	gst_element_link(data.tudpsrc, data.tdepay);
	gst_element_link(data.tdepay,data.tdecoder);
	gst_element_link_many(data.tdecoder,data.vconvert, data.isink,NULL);

	/* setting pipeline to playing state */
	ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Unable to set the pipeline to the playing state.\n");
		gst_object_unref(data.pipeline);
		return -1;
	}

	ret1 = gst_element_set_state(data.tpipeline, GST_STATE_PLAYING);
	if (ret1 == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Unable to set the pipeline to the playing state.\n");
		gst_object_unref(data.tpipeline);

		return -1;
	}

	bus = gst_element_get_bus(data.pipeline);
	msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
	msg1 = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

	if(msg != NULL) {
		GError *err;
		gchar *debug_info;
		switch(GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_ERROR:
			gst_message_parse_error (msg, &err, &debug_info);
			g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
			g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
			g_clear_error (&err);
			g_free (debug_info);

			terminate = true;
			break;
		case GST_MESSAGE_EOS:
			g_print ("End-Of-Stream reached.\n");
			terminate = true;
			break;
		default:
			g_printerr ("Unexpected message received.\n");
			break;
		}
		gst_message_unref (msg);
	}

	gst_object_unref(bus);
	gst_element_set_state(data.pipeline, GST_STATE_NULL);
	gst_object_unref(data.pipeline);

	gst_element_set_state(data.tpipeline, GST_STATE_NULL);
	gst_object_unref(data.tpipeline);
	return 0;

}











