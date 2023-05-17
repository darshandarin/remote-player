#include<iostream>
#include <gst/gst.h>
#include "tut_header.h"

using namespace std;

static void pad_added_handler(GstElement *src, GstPad *pad, gpointer *data)
{
	  GstElement * sink= (GstElement *) data;
	  GstCaps *new_pad_caps = NULL;
	  GstStructure *new_pad_struct = NULL;
	  const gchar *new_pad_type = NULL;
	  GstPadLinkReturn ret;
	  GstPad *sinkpad;
	  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (pad), GST_ELEMENT_NAME (src));
/* Check the new pad's type */
	  new_pad_caps = gst_pad_get_current_caps (pad);
	  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
	  new_pad_type = gst_structure_get_name (new_pad_struct);
	  if (g_str_has_prefix (new_pad_type, "audio")) {
		  g_print ("It has type '%s'\n", new_pad_type);
		  sinkpad = gst_element_get_static_pad(sink, "sink");
		  ret = gst_pad_link (pad, sinkpad);
		  if (GST_PAD_LINK_FAILED (ret))
			  g_print ("Type is '%s' but link failed.\n", new_pad_type);
		  else
			  g_print ("Link succeeded (type '%s').\n", new_pad_type);
 }
	  else if (g_str_has_prefix (new_pad_type, "video")) {
		  g_print ("It has type '%s'\n", new_pad_type);
		  sinkpad = gst_element_get_static_pad(sink, "sink");
		  ret = gst_pad_link (pad, sinkpad);
		  if (GST_PAD_LINK_FAILED (ret)){
			  g_print ("Type is '%s' but link failed.\n", new_pad_type);
	   }
	   else{
		   g_print ("Link succeeded (type '%s').\n", new_pad_type);
       }
	  }
	  gst_object_unref(sinkpad);
}






int thumbnail(string filename) {
  GstElement *pipeline, *filesrc, *decodebin, *tee, *queue1, *queue2, *videoconvert, *videoscale, *capsfilter, *jpegenc, *filesink, *autosink;
  GstPad *tee_pad1, *tee_pad2, *queue_pad1, *queue_pad2;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;


  /* Initialize GStreamer */
  gst_init(NULL, NULL);

  /* Create elements */
  filesrc = gst_element_factory_make("filesrc", "file-source");
  decodebin = gst_element_factory_make("decodebin", "decode-bin");
  videoconvert = gst_element_factory_make("videoconvert", "video-convert");


  videoscale = gst_element_factory_make("videoscale", "video-scale");
  capsfilter = gst_element_factory_make("capsfilter", "caps-filter");
  jpegenc = gst_element_factory_make("jpegenc", "jpeg-encoder");
  filesink = gst_element_factory_make("filesink", "file-sink");

  /* Create pipeline */
  pipeline = gst_pipeline_new("video-thumbnail-pipeline");

  /* Check if all elements were created successfully */
  if (!filesrc || !decodebin || !videoconvert || !tee || !queue1 || !queue2 || !videoscale || !capsfilter || !jpegenc || !filesink || !autosink || !pipeline) {
    g_printerr("Failed to create GStreamer elements\n");
    return -1;
  }
  // gchar* uri = (gchar *) "/home/ee212870/Downloads/";
  // uri = g_strconcat(uri,filename,NULL);
  string uri = "/home/ee212829/Desktop/darshan/pro1/"+filename;
  cout<<uri<<endl;

  /* Set file source location */
  g_object_set(G_OBJECT(filesrc), "location", uri.c_str(), NULL);

  /* Set video scale properties */
  g_object_set(G_OBJECT(videoscale), "method", 0, "add-borders", FALSE, NULL);
  g_object_set(G_OBJECT(capsfilter), "caps", gst_caps_new_simple("video/x-raw", "width", G_TYPE_INT, 240, "height", G_TYPE_INT, 135, NULL), NULL);

  /* Set file sink location */
  g_object_set(G_OBJECT(filesink), "location", "/home/ee212941/Downloads/ass6/abc.jpg", NULL);

  /* Add elements to pipeline */
  gst_bin_add_many(GST_BIN(pipeline), filesrc, decodebin, videoconvert, videoscale, capsfilter, jpegenc, filesink,NULL);

  // Link elements

if (!gst_element_link(filesrc, decodebin)) {
    g_printerr("Failed to link filesrc and decodebin\n");
    gst_object_unref(pipeline);
    return -1;
  }

   g_signal_connect(decodebin,"pad-added",G_CALLBACK(pad_added_handler),videoconvert);

   if (!gst_element_link(videoconvert, videoscale)) {
       g_printerr("Failed to link videoconvert and tee\n");
       gst_object_unref(pipeline);
       return -1;
     }




  if (!gst_element_link(videoscale, capsfilter)) {
    g_printerr("Failed to link videoscale and capsfilter\n");
    gst_object_unref(pipeline);
    return -1;
  }


  if (!gst_element_link(capsfilter, jpegenc)) {
    g_printerr("Failed to link capsfilter and jpegenc\n");
    gst_object_unref(pipeline);
    return -1;
  }

  if (!gst_element_link(jpegenc, filesink)) {
    g_printerr("Failed to link jpegenc and filesink\n");
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
 do {
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      (GstMessageType)(  GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* Parse message */
    if (msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error (msg, &err, &debug_info);
          g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
          g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
          g_clear_error (&err);
          g_free (debug_info);
          terminate = TRUE;
          break;
        case GST_MESSAGE_EOS:
          g_print ("End-Of-Stream reached.\n");
          terminate = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          /* We are only interested in state-changed messages from the pipeline */
          if (GST_MESSAGE_SRC (msg) == GST_OBJECT (pipeline)) {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
            g_print ("Pipeline state changed from %s to %s:\n",
                gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
          }
          break;
        default:
          /* We should not reach here */
          g_printerr ("Unexpected message received.\n");
          break;
      }
      gst_message_unref (msg);
    }
  } while (!terminate);

  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}


int thumbnail_main(int argc, char* argv[]){
	thumbnail("ass6/vid2.mp4");
}





