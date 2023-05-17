#include <gst/gst.h>
#include<string>
#include<vector>
#include <cstdlib>
#include<iostream>
#include <glib-unix.h>
#include<gst/pbutils/pbutils.h>
#include <signal.h>
using namespace std;
static char* codec=NULL;
gint flag=0;

typedef struct _Metaadata {
	GstDiscoverer *discoverer;
	GMainLoop *loop;
} CustomData1;

typedef struct _CustomData {
	GstElement *pipeline, *filesrc, *qtdemux, *videoconvert, *x264enc,*rtph264pay, *udpsink;
	GstElement *audioconvert, *audioresample, *aencoder, *pay, *udpsink1, *videobalance;
	GstElement *vqueue, *aqueue, *vdecoder, *adecoder, *vdecoder2, *adecoder2, *videoscale;
	GstElement *vparse, *vparse2, *aparse, *adecoder1, *vdecoder1, *mpeg2enc, *filter;
	gboolean playing, quit, seek;
	GstElement *volume;
	GMainLoop *loop;
	gint64 duration;
	gdouble len;
	gint64 current_position;
	gchar *vcodec_type;
	string video_path;
	int thumb_image_number=1;
	GstElement *pipeline1,*filesrc1,*decoder1,*vconvert1,*vscale1, *ifreeze1, *encoder1,*vpay1,*udpsink2;
} CustomData;

/* this function fetches the file from given location and displays the thumbnail in remote machine*/
static void play_thumbnail(gchar *uri,CustomData *data){
	GstMessage *message;
	GstBus *bus;
	GstStateChangeReturn ret;
	gboolean terminate=false;
	data->pipeline1 = gst_pipeline_new("thumb_pipeline");
	data->filesrc1 = gst_element_factory_make("filesrc", "src1");

	/*fetching tumbnail from the following location */
	g_object_set(G_OBJECT(data->filesrc1), "location", "/home/ee212829/Downloads/thumbnail/thumbnail.jpg", NULL);

	/*creating elements for thumbnail*/
	data->decoder1=gst_element_factory_make("jpegdec","jdec");
	data->vconvert1=gst_element_factory_make("videoconvert", "vconver");
	data->vscale1=gst_element_factory_make("videoscale", "scale");
	data->ifreeze1=gst_element_factory_make("imagefreeze", "ifreeze");
	g_object_set(G_OBJECT(data->ifreeze1), "num-buffers",40, NULL);
	data->encoder1 = gst_element_factory_make("jpegenc", "enc");
	data->vpay1 = gst_element_factory_make("rtpjpegpay", "payloader");
	data->udpsink2 = gst_element_factory_make("udpsink", "sink2");

	/* setting the port number to display thumbnail in remote machine */
	g_object_set(G_OBJECT(data->udpsink2), "host", "10.1.138.186", "port",5555, NULL);

	/* adding elements to the pipeline */
	gst_bin_add_many(GST_BIN(data->pipeline1), data->filesrc1,data->decoder1, data->vconvert1, data->vscale1,data->ifreeze1,
			data->encoder1, data->vpay1,data->udpsink2,NULL);

	/* linking elements */
	gst_element_link(data->filesrc1, data->decoder1);
	gst_element_link(data->decoder1, data->vconvert1);
	gst_element_link_many(data->vconvert1,data->vscale1,data->ifreeze1, data->encoder1, data->vpay1,data->udpsink2,NULL);

	/*set pipeline to playing state */
	ret = gst_element_set_state(data->pipeline1, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Unable to set the pipeline to the playing state.\n");
		gst_object_unref(data->pipeline);
		;
	}

	bus = gst_element_get_bus(data->pipeline1);
	message = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

	if(message!=NULL){
		switch (GST_MESSAGE_TYPE(message)) {
		case GST_MESSAGE_EOS:
			g_print("End of stream\n");
			gst_element_set_state(data->pipeline1, GST_STATE_NULL);
			break;
		case GST_MESSAGE_ERROR: {
			gchar *debug;
			GError *error;

			gst_message_parse_error(message, &error, &debug);
			g_free(debug);
			g_printerr("Error: %s\n", error->message);
			g_error_free(error);
			break;
		}
		gst_message_unref(message);
		}
		gst_element_set_state(data->pipeline1, GST_STATE_NULL);
		gst_object_unref(bus);
		gst_object_unref(data->pipeline1);

	}
}

/* Print a tag in a human-readable format (name: value) */
static void print_tag_foreach (const GstTagList *tags, const gchar *tag, gpointer user_data) {
	GValue val = { 0, };
	gchar *str;
	gint depth = GPOINTER_TO_INT (user_data);

	gst_tag_list_copy_value (&val, tags, tag);

	if (G_VALUE_HOLDS_STRING (&val))
		str = g_value_dup_string (&val);
	else
		str = gst_value_serialize (&val);

	if(strcmp("QT atom",gst_tag_get_nick (tag))!=0)
		g_print ("\t\t\t%*s%s: %s\n", 2 * depth, " ", gst_tag_get_nick (tag), str);
	g_free (str);

	g_value_unset (&val);
}


/* Print information regarding a stream and its substreams, if any */
static void print_topology (GstDiscovererStreamInfo *info, gint depth) {
	GstDiscovererStreamInfo *next;

	if (!info)
		return;

	next = gst_discoverer_stream_info_get_next (info);
	if (next) {
		print_topology (next, depth + 1);
		gst_discoverer_stream_info_unref (next);
	}
	else if (GST_IS_DISCOVERER_CONTAINER_INFO (info)) {
		GList *tmp, *streams;
		streams = gst_discoverer_container_info_get_streams (GST_DISCOVERER_CONTAINER_INFO (info));
		for (tmp = streams; tmp; tmp = tmp->next) {
			GstDiscovererStreamInfo *tmpinf = (GstDiscovererStreamInfo *) tmp->data;
			print_topology (tmpinf, depth + 1);
		}
		gst_discoverer_stream_info_list_free (streams);
	}
}

/* This function is called every time the discoverer has information regarding one of the URIs we provided.*/
static void on_discovered_cb (GstDiscoverer *discoverer, GstDiscovererInfo *info, GError *err, CustomData *data) {
	GstDiscovererResult result;
	const gchar *uri;
	const GstTagList *tags;
	GstDiscovererStreamInfo *sinfo;

	uri = gst_discoverer_info_get_uri (info);
	result = gst_discoverer_info_get_result (info);
	switch (result) {
	case GST_DISCOVERER_URI_INVALID:
		g_print ("Invalid URI '%s'\n", uri);
		break;
	case GST_DISCOVERER_ERROR:
		g_print ("Discoverer error: %s\n", err->message);
		break;
	case GST_DISCOVERER_TIMEOUT:
		g_print ("Timeout\n");
		break;
	case GST_DISCOVERER_BUSY:
		g_print ("Busy\n");
		break;
	case GST_DISCOVERER_MISSING_PLUGINS:{
		const GstStructure *s;
		gchar *str;

		s = gst_discoverer_info_get_misc (info);
		str = gst_structure_to_string (s);

		g_print ("Missing plugins: %s\n", str);
		g_free (str);
		break;
	}
	case GST_DISCOVERER_OK:
		g_print ("METADATA OF THE FILE  '%s'\n\n", uri);
		break;
	}

	if (result != GST_DISCOVERER_OK) {
		g_printerr ("This URI cannot be played\n");
		return;
	}


	/* If we got no error, show the retrieved information */
	tags = gst_discoverer_info_get_tags (info);
	if (tags) {
		//g_print ("Tags:\n");
		gst_tag_list_foreach (tags, print_tag_foreach, GINT_TO_POINTER (1));
	}
	g_print ("\t\t\t  Duration: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS (gst_discoverer_info_get_duration (info)));

	g_print ("\t\t\t  Seekable: %s\n", (gst_discoverer_info_get_seekable (info) ? "yes" : "no"));

	g_print ("\n");

	sinfo = gst_discoverer_info_get_stream_info (info);
	if (!sinfo)
		return;


	print_topology (sinfo, 1);

	gst_discoverer_stream_info_unref (sinfo);

	g_print ("\n");
}

/* This function is called when the discoverer has finished examining all the URIs we provided.*/
static void on_finished_cb (GstDiscoverer *discoverer, CustomData *data) {
	g_print ("--------------------------------------------------------\n");

}

/* pad handler function which connects to the video converter */
static void pad_added_handler1(GstElement *src, GstPad *new_pad,GstElement *videoconvert)
{

	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;

	/* Check the new pad's type */
	new_pad_caps = gst_pad_get_current_caps(new_pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type = gst_structure_get_name(new_pad_struct);
	//g_print("%s  \n", new_pad_type);

	if (g_str_has_prefix(new_pad_type, "video/"))
	{

		GstPad *sink_pad1 = gst_element_get_static_pad(videoconvert, "sink");
		gst_pad_link(new_pad, sink_pad1);
		gst_object_unref(sink_pad1);
	}
}

/* this function is to generate thumbnail */
int thumbnail(string uri) {
	GstElement *pipeline, *filesrc, *decodebin,  *videoconvert, *videoscale;
	GstElement *capsfilter, *jpegenc, *filesink,*videorate;
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;
	gboolean terminate = FALSE;

	/* Create elements */
	filesrc = gst_element_factory_make("filesrc", "file-source");
	decodebin = gst_element_factory_make("decodebin", "decode-bin");
	videoconvert = gst_element_factory_make("videoconvert", "video-convert");
	videorate = gst_element_factory_make("videorate", "videorate");
	videoscale = gst_element_factory_make("videoscale", "video-scale");
	capsfilter = gst_element_factory_make("capsfilter", "caps-filter");
	jpegenc = gst_element_factory_make("jpegenc", "jpeg-encoder");
	filesink = gst_element_factory_make("filesink", "file-sink");

	/* Create pipeline */
	pipeline = gst_pipeline_new("video-thumbnail-pipeline");

	/* Check if all elements were created successfully */
	if (!pipeline || !filesrc || !decodebin || !videoconvert || !videorate  || !videoscale || !capsfilter || !jpegenc || !filesink ) {
		g_printerr("Failed to create GStreamer elements\n");
		return -1;
	}

	/* Set file source location */
	g_object_set(G_OBJECT(filesrc), "location", uri.c_str(), NULL);

	/* Set video scale properties */
	g_object_set(capsfilter, "caps", gst_caps_new_simple("video/x-raw","framerate",GST_TYPE_FRACTION,1,100, "width", G_TYPE_INT, 400, "height", G_TYPE_INT, 550, NULL), NULL);

	/* Set file sink location */
	g_object_set(G_OBJECT(filesink), "location", "/home/ee212829/Downloads/thumbnail/thumbnail.jpg", NULL);

	/* Add elements to pipeline */
	gst_bin_add_many(GST_BIN(pipeline), filesrc, decodebin, videoconvert,videorate, videoscale, capsfilter, jpegenc, filesink,NULL);

	// Link elements
	if (!gst_element_link(filesrc, decodebin)) {
		g_printerr("Failed to link filesrc and decodebin\n");
		gst_object_unref(pipeline);
		return -1;
	}

	g_signal_connect(decodebin,"pad-added",G_CALLBACK(pad_added_handler1),videoconvert);

	if (!gst_element_link_many(videoconvert, videoscale,videorate,capsfilter,jpegenc,filesink,NULL)) {
		g_printerr("thumbnail Linking failed\n");
		gst_object_unref(pipeline);
		return -1;
	}

	/* Set pipeline state to playing */
	ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Failed to set pipeline state to playing\n");
		gst_object_unref(pipeline);
		return -1;
	}

	/* Wait for pipeline to finish */
	bus = gst_element_get_bus(pipeline);

	msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
			(GstMessageType)(  GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

	/* Parse message */
	if (msg != NULL) {}

	gst_object_unref(bus);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);

	return 0;
}


// Callback function to handle messages from the bus
static gboolean bus_callback(GstBus *bus, GstMessage *message, CustomData *data)

{
	switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_EOS:
		g_print("End of stream\n");
		gst_element_set_state(data->pipeline, GST_STATE_NULL);
		g_main_loop_quit(data->loop);
		break;
	case GST_MESSAGE_ERROR: {
		gchar *debug;
		GError *error;

		gst_message_parse_error(message, &error, &debug);
		g_free(debug);
		g_printerr("Error: %s\n", error->message);
		g_error_free(error);
		break;
	}
	case GST_MESSAGE_WARNING: {
		gchar *debug;
		GError *error;

		gst_message_parse_warning(message, &error, &debug);
		g_free(debug);
		g_printerr("Warning: %s\n", error->message);
		g_error_free(error);
		break;
	}
	case GST_MESSAGE_STATE_CHANGED:
		/* We are only interested in state-changed messages from the pipeline */
		if (GST_MESSAGE_SRC (message) == GST_OBJECT (data->pipeline)) {
			GstState old_state, new_state, pending_state;
			gst_message_parse_state_changed (message, &old_state, &new_state, &pending_state);
			g_print ("Pipeline state changed from %s to %s:\n",
					gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
		}
		break;

	case GST_MESSAGE_ELEMENT: {
		const GstStructure *structure = gst_message_get_structure(message);
		if (gst_structure_has_name(structure, "GstBinForwarded")) {
			GstMessage *child_message;
			gst_structure_get(structure, "message", GST_TYPE_MESSAGE,
					&child_message, NULL);
			if (child_message) {
				bus_callback(bus, child_message, data);
				gst_message_unref(child_message);
			}
		}
		break;
	}
	default:
		// 	g_print("Received message type %s\n", GST_MESSAGE_TYPE_NAME(message));
		break;

	}

	return TRUE;
}


static void on_new_pad(GstElement *dec, GstPad *pad, GstElement *fakesink) {
	GstPad *sinkpad;
	sinkpad = gst_element_get_static_pad(fakesink, "sink");
	if (!gst_pad_is_linked(sinkpad)) {
		if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK)
			g_error("Failed to link pads!");
	}
	gst_object_unref(sinkpad);
}

/* this function is to fetch the particular codec information */
gchar* Retrieve_Codec(gchar *uri, CustomData *data) {
	GstElement *pipe, *dec, *sink;
	GstMessage *msg;
	gchar *codec;

	pipe = gst_pipeline_new("pipeline");
	dec = gst_element_factory_make("uridecodebin", NULL);
	g_object_set(dec, "uri", uri, NULL);
	gst_bin_add(GST_BIN(pipe), dec);

	sink = gst_element_factory_make("fakesink", NULL);
	gst_bin_add(GST_BIN(pipe), sink);

	g_signal_connect(dec, "pad-added", G_CALLBACK (on_new_pad), sink);

	gst_element_set_state(pipe, GST_STATE_PAUSED);

	GstTagList *tags = NULL;
	while (TRUE) {
		msg = gst_bus_timed_pop_filtered(GST_ELEMENT_BUS(pipe),
				GST_CLOCK_TIME_NONE,
				(GstMessageType) (GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_TAG
						| GST_MESSAGE_ERROR));

		if (GST_MESSAGE_TYPE (msg) != GST_MESSAGE_TAG)
			break;

		gst_message_parse_tag(msg, &tags);

		gst_tag_list_get_string(tags, GST_TAG_VIDEO_CODEC, &codec);
	}
	gst_tag_list_unref(tags);

	gst_message_unref(msg);
	gst_element_set_state(pipe, GST_STATE_NULL);
	gst_object_unref(pipe);
	return codec;
}

/* this function is to print the metadata information of the file */
static void metadata(gchar *uri)
{

	CustomData1 data1;
	GError *err = NULL;
	g_print ("----------------------------------------------------------\n");

	/* Instantiate the Discoverer */
	data1.discoverer = gst_discoverer_new (3 * GST_SECOND, &err);
	if (!data1.discoverer) {
		g_print ("Error creating discoverer instance: %s\n", err->message);
		g_clear_error (&err);
		return ;
	}
	/* Connect to the interesting signals */
	g_signal_connect (data1.discoverer, "discovered", G_CALLBACK (on_discovered_cb), &data1);
	g_signal_connect (data1.discoverer, "finished", G_CALLBACK (on_finished_cb), &data1);

	/* Start the discoverer process  */
	gst_discoverer_start (data1.discoverer);

	/* Add a request to process asynchronously the URI passed through the command line */
	if (!gst_discoverer_discover_uri_async (data1.discoverer, uri)) {
		g_print ("Failed to start discovering URI '%s'\n", uri);
		g_object_unref (data1.discoverer);
		return  ;
	}
}

/* pad handler function */
static void pad_added_handler(GstElement *src, GstPad *new_pad,CustomData *data) {
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;

	/* Check the new pad's type */
	new_pad_caps = gst_pad_get_current_caps(new_pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type = gst_structure_get_name(new_pad_struct);


	if (g_str_has_prefix(new_pad_type, "video/"))
	{

		GstPad *sink_pad1 = gst_element_get_static_pad(data->vqueue, "sink");
		gst_pad_link(new_pad, sink_pad1);
		gst_object_unref(sink_pad1);

	}

	else if (g_str_has_prefix(new_pad_type, "audio/"))
	{

		GstPad *sink_pad1 = gst_element_get_static_pad(data->aqueue, "sink");
		gst_pad_link(new_pad, sink_pad1);
		gst_object_unref(sink_pad1);

	}
}


/* function to take inputs from the keyboard */
static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond,CustomData *data) {
	gsize length = 0;
	GError *error = NULL;
	gchar *str = NULL;
	GstState state, pending;
	gint64 position;

	if (g_io_channel_read_line(source, &str, &length, NULL,&error) != G_IO_STATUS_NORMAL) {
		return TRUE;
	}

	switch (g_ascii_tolower(str[0]))
	{

	/* takes input 'p' to toggle between play and pause */
	case 'p':
		data->playing = !data->playing;
		gst_element_set_state(data->pipeline,data->playing ? GST_STATE_PLAYING : GST_STATE_PAUSED);
		g_print("Setting state to %s \n", data->playing ? "PLAYING" : "PAUSE");
		break;

		/* takes input 'f' for forward seek */
	case 'f':
		gst_element_query_position(data->pipeline, GST_FORMAT_TIME,&data->current_position);
		gst_element_query_duration(data->pipeline, GST_FORMAT_TIME,&data->duration);
		g_print("position = %" GST_TIME_FORMAT "/%" GST_TIME_FORMAT "\n", GST_TIME_ARGS(data->current_position), GST_TIME_ARGS(data->duration));


		if (data->current_position < data->duration)
		{
			gst_element_seek_simple(data->pipeline, GST_FORMAT_TIME,(GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),data->current_position + (10 * GST_SECOND));

		}
		break;

		/* takes input 'b' for backward seek */
	case 'b':
		gst_element_query_position(data->pipeline, GST_FORMAT_TIME,&data->current_position);
		gst_element_query_duration(data->pipeline, GST_FORMAT_TIME,&data->duration);

		// g_print("position=  %" GST_TIME_FORMAT "/ %" GST_TIME_FORMAT "\n",
		//          GST_TIME_ARGS (data->current_position), GST_TIME_ARGS (data->duration));
		//cout << data->current_position << "\t" << 10 * GST_SECOND << "\n";
		if (data->current_position <= 10 * GST_SECOND)
		{
			gst_element_seek_simple(data->pipeline, GST_FORMAT_TIME,(GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
					0 * GST_SECOND);
			gst_element_query_position(data->pipeline, GST_FORMAT_TIME,&data->current_position);
			gst_element_query_duration(data->pipeline, GST_FORMAT_TIME,&data->duration);
		}
		else
		{
			gst_element_seek_simple(data->pipeline, GST_FORMAT_TIME,(GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),data->current_position - (10 * GST_SECOND));
			gst_element_query_position(data->pipeline, GST_FORMAT_TIME,&data->current_position);
			gst_element_query_duration(data->pipeline, GST_FORMAT_TIME,&data->duration);
		}
		//g_print("position =  %" GST_TIME_FORMAT "/ %" GST_TIME_FORMAT "\n",GST_TIME_ARGS (data->current_position),GST_TIME_ARGS (data->duration));
		break;

		/* takes input 'i' to increase the volume */
	case 'i':
		gst_element_get_state(data->pipeline, &state, &pending, 0);
		if (state == GST_STATE_PLAYING) {
			gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
		}
		g_object_get(data->volume, "volume", &data->len, NULL);
		if (data->len == 1.0) {
			g_print("Volume is already at maximum\n");
		} else
		{
			data->len = MIN(data->len + 0.1, 1.0);
			g_object_set(G_OBJECT(data->volume), "volume", data->len, NULL);
			g_print("Volume increased to: %.1f\n", data->len);
		}
		gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
		break;

		/* takes input 'd' to decrease the volume */
	case 'd':
		gst_element_get_state(data->pipeline, &state, &pending, 0);
		if (state == GST_STATE_PLAYING) {
			gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
		}
		g_object_get(G_OBJECT(data->volume), "volume", &data->len, NULL);
		if (data->len == 0.0) {
			g_print("Volume is already at minimum\n");
		}
		else {
			data->len = MAX(data->len - 0.1, 0.0);
			g_object_set(G_OBJECT(data->volume), "volume", data->len, NULL);
			g_print("Volume decreased to: %.1f\n", data->len);
		}
		gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
		break;

		/* takes input 'n' to flush */
	case 'n':
		GstEvent *start,*stop;
		if(start = gst_event_new_flush_start())
		{
			g_print("flush applied\n");
		}
		gst_element_send_event(data->pipeline, start);
		gst_element_set_state(data->pipeline, GST_STATE_NULL);
		stop = gst_event_new_flush_stop(true);
		gst_element_send_event(data->pipeline, stop);
		data->quit = true;
		g_main_loop_quit(data->loop);
		break;

	default:
		g_print("Enter valid input \n");
		break;

	}

	g_free(str);
	return TRUE;
}

//GST pad Probe Returns for play
static GstPadProbeReturn on_pad_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
	GstElement *pipeline = GST_ELEMENT(user_data);

	if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_DATA(info)) == GST_EVENT_STREAM_START) {
		g_print("Stream started!\n");
		gst_element_set_state(pipeline, GST_STATE_PLAYING);
	}

	return GST_PAD_PROBE_OK;
}
// GST_Pad_Probe_Return for EOS EVENT
static GstPadProbeReturn my_probe (GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
	GstEvent *event = GST_EVENT(info->data);
	GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

	switch (GST_EVENT_TYPE(event)) {
	case GST_EVENT_EOS:
		g_print("Received EOS event\n");
		break;
	default:
		break;
	}

	return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn cb_have_data(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
	CustomData* data = static_cast<CustomData*>(user_data);
	//for setting new width and height
	GstCaps* filtercaps = gst_caps_new_simple("video/x-raw",
			"width", G_TYPE_INT,1080,
			"height", G_TYPE_INT,1080,
			NULL);
	g_object_set(G_OBJECT(data->filter), "caps", filtercaps, NULL);
	gst_caps_unref(filtercaps);
	//for setting the contrast
	g_object_set(G_OBJECT(data->videobalance), "contrast", 0.4, NULL);
	// Set the grayscale effect
	// g_object_set(G_OBJECT(data->videobalance), "saturation", 0.0, NULL);
	return GST_PAD_PROBE_OK;
}

/* main function */
int main(int argc, char *argv[])
{
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;
	GIOChannel *io_stdin;
	GError *err = NULL;
	GstMessage *msg1;
	gboolean terminate = FALSE;
	CustomData data;
	vector<string> videos;



	// Initialize GStreamer
	gst_init(&argc, &argv);

	string path = "/home/ee212829/Desktop/darshan/pro1/"; // Replace with the actual path to your directory

	DIR *dir;
	struct dirent *ent;

	if ((dir = opendir(path.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			string filename = ent->d_name;
			if (filename.find_last_of(".") != string::npos) {
				string extension = filename.substr(
						filename.find_last_of("."));
				if (extension == ".mp4") {
					videos.push_back(path + filename);
				}
			}
		}
		closedir(dir);
	}
#ifdef G_OS_WIN32
	io_stdin = g_io_channel_win32_new_fd (fileno (stdin));
#else
	io_stdin = g_io_channel_unix_new(fileno(stdin));

#endif
	g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc) handle_keyboard, &data);
	data.loop = g_main_loop_new(NULL, FALSE);

	for (auto i : videos) {
		data.video_path= i;
		thumbnail(data.video_path);
		g_print(
				"\nUSAGE: Choose one of the following options, then press enter:\n"
				"  'P' for PAUSE and PLAY\n"
				"  'I' for volume increament\n "
				"  'D' for volume decreament\n "
				"  'F' for Forward Seek\n"
				"  'B' for Backword seek\n"
				"  'N' for Flush and Next Video\n");


		string s = "file://" + i;
		gchar *uri = (gchar*) s.c_str();
		gst_init(&argc, &argv);

		metadata(uri);



		data.vcodec_type = Retrieve_Codec(uri,&data);

		data.pipeline = gst_pipeline_new("pipeline");
		data.filesrc = gst_element_factory_make("filesrc", "file-source");
		g_object_set(G_OBJECT(data.filesrc), "location", i.c_str(), NULL);
		data.qtdemux = gst_element_factory_make("qtdemux", "qt");
		data.vparse = gst_element_factory_make("mpeg4videoparse", "parse");
		data.vparse2 = gst_element_factory_make("mpegvideoparse", "parse2");
		data.aparse = gst_element_factory_make("mpegaudioparse", "aparse");
		data.vqueue = gst_element_factory_make("queue", "vqueue");
		data.aqueue = gst_element_factory_make("queue", "aqueue");
		data.vdecoder = gst_element_factory_make("avdec_mpeg4", "vdecoder");
		data.vdecoder1 = gst_element_factory_make("avdec_h264", "avdec_h264");
		data.vdecoder2 = gst_element_factory_make("avdec_mpeg2video","avdec_mpeg2video");
		data.adecoder = gst_element_factory_make("avdec_mp3", "avdec_mp3");
		data.adecoder1 = gst_element_factory_make("faad", "faad");
		data.adecoder2 = gst_element_factory_make("avdec_mp2float","avdec_mp2float");
		data.videoconvert = gst_element_factory_make("videoconvert","converter");
		data.videoscale = gst_element_factory_make("videoscale", "video-scaler");
		data.filter = gst_element_factory_make("capsfilter", "filter");
		data.videobalance=gst_element_factory_make("videobalance","videobalance");
		data.x264enc = gst_element_factory_make("x264enc", "encoder");
		g_object_set(G_OBJECT(data.x264enc), "tune", 4, NULL);
		data.rtph264pay = gst_element_factory_make("rtph264pay", "payloader");
		data.udpsink = gst_element_factory_make("udpsink", "network-sink");
		data.audioconvert = gst_element_factory_make("audioconvert","audioconvert");
		data.volume = gst_element_factory_make("volume", "vol1");
		g_object_set((data.volume), "volume", 0.1, NULL);
		data.audioresample = gst_element_factory_make("audioresample","audioresample");
		data.aencoder = gst_element_factory_make("opusenc", "aencoder");
		data.pay = gst_element_factory_make("rtpopuspay", "pay");
		data.udpsink1 = gst_element_factory_make("udpsink", "udpsink1");
		g_object_set(G_OBJECT(data.udpsink), "host", "10.1.138.186", "port",5678, NULL);
		g_object_set(G_OBJECT(data.udpsink1), "host", "10.1.138.186", "port",5679, NULL);

		if (!data.pipeline || !data.filesrc || !data.vdecoder || !data.volume|| !data.adecoder|| !data.qtdemux ||
				!data.vqueue || !data.aqueue|| !data.videoconvert || !data.vparse || !data.aparse || !data.videobalance ||
				!data.audioresample ||  !data.videoscale || !data.filter || !data.audioconvert || !data.udpsink1 || !data.x264enc||
				!data.rtph264pay || !data.adecoder1|| !data.adecoder2 || !data.vdecoder1 || !data.vdecoder2|| !data.udpsink) {

			g_printerr("Not all elements could be created for video.\n");
			return -1;
		}
		gst_bin_add_many(GST_BIN(data.pipeline), data.filesrc, data.qtdemux,
				data.vqueue, data.aqueue, data.vdecoder, data.vdecoder2,
				data.videobalance,data.videoscale,data.filter,
				data.adecoder, data.adecoder2, data.vdecoder1, data.adecoder1,
				data.aparse, data.vparse, data.videoconvert, data.x264enc,
				data.rtph264pay, data.udpsink, data.audioconvert, data.vparse2,
				data.audioresample, data.pay, data.aencoder, data.volume, data.udpsink1,NULL);

		gst_element_link(data.filesrc, data.qtdemux);

		g_signal_connect(data.qtdemux, "pad-added",G_CALLBACK (pad_added_handler), &data);

		g_signal_connect(data.qtdemux, "pad-added",G_CALLBACK (pad_added_handler), &data);


		if (g_str_has_prefix(data.vcodec_type, "MPEG-4")) {
			if (!gst_element_link_many(data.vqueue, data.vparse, data.vdecoder,
					data.videoconvert, data.videobalance,data.videoscale,data.filter, data.x264enc, data.rtph264pay,
					data.udpsink, NULL)) {
				g_print("mpeg4 video linking fail");
			}
			if (!gst_element_link_many(data.aqueue, data.aparse, data.adecoder,
					data.audioconvert,data.volume, data.audioresample, data.aencoder,
					data.pay, data.udpsink1, NULL)) {
				g_print("mpeg4 audio linking fail");
			}
		}

		if (g_str_has_prefix(data.vcodec_type, "H.264")) {
			if (!gst_element_link_many(data.vqueue, data.vdecoder1,
					data.videoconvert, data.videobalance,data.videoscale,data.filter, data.x264enc, data.rtph264pay,
					data.udpsink, NULL)) {
				g_print("avdec_h264 video linking fail");
			}

			if (!gst_element_link_many(data.aqueue, data.adecoder1,
					data.audioconvert,data.volume, data.audioresample, data.aencoder,
					data.pay, data.udpsink1, NULL)) {
				g_print("avdec_h264 audio linking fail");
			}
		}
		if (g_str_has_prefix(data.vcodec_type, "MPEG-2")) {
			if (!gst_element_link_many(data.vqueue, data.vparse2,
					data.vdecoder2, data.videoconvert, data.videobalance,data.videoscale,data.filter, data.x264enc,
					data.rtph264pay, data.udpsink, NULL)) {
				g_print("mpeg2 video linking fail");
			}

			if (!gst_element_link_many(data.aqueue, data.aparse, data.adecoder2,
					data.audioconvert,data.volume, data.audioresample, data.aencoder,
					data.pay, data.udpsink1, NULL)) {
				g_print("mpeg2 audio linking fail");
			}

		}

		// Get the sink pad of the video sink element
		GstPad *vsource_pad = gst_element_get_static_pad(data.filesrc, "src");
		// Add a pad probe to the sink pad
		gst_pad_add_probe (vsource_pad, GST_PAD_PROBE_TYPE_BUFFER,(GstPadProbeCallback) cb_have_data, &data, NULL);

		// add a probe to the sink pad of the autovideosink element
		GstPad *vsink_pad = gst_element_get_static_pad(data.udpsink, "sink");
		gst_pad_add_probe(vsink_pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,(GstPadProbeCallback)my_probe, NULL, NULL);
		gst_pad_add_probe(vsink_pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, (GstPadProbeCallback)on_pad_probe, data.pipeline, NULL);

		bus = gst_element_get_bus(data.pipeline);
		gst_bus_add_watch(bus, (GstBusFunc) bus_callback, &data);
		gst_object_unref(bus);

		ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
		if (ret == GST_STATE_CHANGE_FAILURE) {
			g_printerr("Unable to set the pipeline to the playing state.\n");
			gst_object_unref(data.pipeline);
			return -1;
		}


		g_main_loop_run(data.loop);
		g_main_loop_quit(data.loop);
		gst_object_unref(data.pipeline);
		play_thumbnail(uri,&data);



	}

	g_io_channel_unref (io_stdin);
	return 0;
}
