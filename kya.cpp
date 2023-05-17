


#include<string>
#include<vector>
#include <gst/gst.h>
#include<iostream>
using namespace std;
//#define GST_LOG();
//GST_DEBUG_CATEGORY_STATIC (my_category);
//#define GST_CAT_DEFAULT my_category;


typedef struct _CustomData {
	GstElement *pipeline;
	GstElement *source,*decodebin;
	GstElement *convert,*vconvert;
	GstElement *resample,*vsample;
	GstElement *sink,*vsink;
	 GstPad *src_pad;
	 gint64 duration;
	gboolean playing;
	 
} CustomData;


static void pad_added_handler(GstElement *src, GstPad *new_pad,
		CustomData *data) {
	GstPad *sink_pad = gst_element_get_static_pad(data->vconvert, "sink");
	gst_pad_link(new_pad, sink_pad);


	/* Unreference the sink pad */
	gst_object_unref(sink_pad);
}
// void perform_flush(GstElement *src) {
//     // Send a flush start event to the source element
//     gst_element_send_event(src, gst_event_new_flush_start());

//     // Wait for the flush to complete
//     GstBus *bus = gst_element_get_bus(src);
//     GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
//             static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR | GST_MESSAGE_STATE_CHANGED));
//     gst_message_unref(msg);

//     // Send a flush stop event to the source element
//     gst_element_send_event(src, gst_event_new_flush_stop(true));
//     g_print("flush completed");

//     // Clean up
//     gst_object_unref(bus);
// }



void seek_to_position(gint64 position,GstElement* data) {
	gst_element_seek_simple(data, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), 10+position);
	}


bool flush_pipeline(CustomData *data) {

	GstBus *bus;
 //GstElement *pipeline = (GstElement *)pipeline;
gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
    // /* Seek to the end of the current media file */
    // if (!gst_element_seek(decodebin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
    //                       GST_SEEK_TYPE_END, 0, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
    // {
    //     g_print("Failed to seek to end of media file\n");
    // }
  //gint j;
// 	gst_element_seek_simple(data->pipeline,GST_FORMAT_TIME,GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),GST_SECOND);
 GstEvent *flush_start=gst_event_new_flush_start();
if( gst_element_send_event(data->pipeline,flush_start))
{
	g_print("fluh start");
}
 gst_element_set_state(data->pipeline, GST_STATE_NULL);
//gst_element_seek_simple(data->pipeline,GST_FORMAT_TIME,GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),GST_SECOND);
 
 GstEvent *flush_stop=gst_event_new_flush_stop(TRUE);
 gst_element_send_event(data->pipeline,flush_stop);
 
	 //*flush_stop=gst_event_new_flush_stop(TRUE);
g_object_set(data->source,"location","new6.mp4",NULL);
gst_element_set_state(data->pipeline, GST_STATE_PLAYING);



// GstEvent *flush_stop=gst_event_new_flush_stop(TRUE);
//  if(gst_element_send_event(data->pipeline,flush_start) && *flush_stop=gst_event_new_flush_stop(TRUE);)
//  {
// 	 cout<<i<<endl;
// 	 set_source(data->source,i);
//  }

	// gst_element_set_state(data->pipeline, GST_STATE_NULL);
//	gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
	
	// bus= gst_element_get_bus(data->pipeline);

	// gst_bus_poll(bus,GST_MESSAGE_FLUSH_START, 0);


	// gst_bus_poll(bus,GST_MESSAGE_FLUSH_STOP, GST_CLOCK_TIME_NONE);
	
	GstSeekFlags flags = GST_SEEK_FLAG_FLUSH;
    GstSeekType start_type = GST_SEEK_TYPE_SET;
    gint64 start_position = 0;
    GstSeekType end_type = GST_SEEK_TYPE_END;
    gint64 end_position = GST_CLOCK_TIME_NONE;

    // gst_element_seek_simple(data->pipeline,GstSeekFlags(GST_SEEK_FLAG_FLUSH),start_type, start_position, end_type, end_position);
  //gst_element_seek_simple(data->pipeline,GstFormat( GST_FORMAT_TIME),GstSeekFlags( GST_SEEK_FLAG_FLUSH), start_position,GstSeekType(GST_SEEK_TYPE_END), end_position);
    //gst_element_seek_simple(data->pipeline, GST_SEEK_FLAG_FLUSH, start_type, start_position, end_type, end_position);

    // /* Stop playback */
    // gst_element_set_state(pipeline, GST_STATE_NULL);

     return true;
 }

 void set_source(GstElement *source,string i)
 {
  g_object_set(source, "location",i.c_str(),NULL);
 // cout<<videos<<endl;
 }

void handle_seek_input(char input,CustomData *data,string i) {
    gint64 current_position,p;
    switch(input) {
        case 's':
            gst_element_query_position(data->pipeline, GST_FORMAT_TIME, &current_position);
            gst_element_query_duration(data->pipeline, GST_FORMAT_TIME, &data->duration);


           g_print("position=  %"GST_TIME_FORMAT "/ %" GST_TIME_FORMAT"\n" ,GST_TIME_ARGS(current_position),GST_TIME_ARGS(data->duration));
            if (current_position < data->duration) {
                seek_to_position(current_position + (3*GST_SECOND),data->pipeline);
            }
            break;
        case 'b':

        	gst_element_query_position(data->pipeline, GST_FORMAT_TIME, &current_position);
        	gst_element_query_duration(data->pipeline, GST_FORMAT_TIME, &data->duration);

        	g_print(" position=  % " GST_TIME_FORMAT "/ %" GST_TIME_FORMAT"\n" ,GST_TIME_ARGS(current_position),GST_TIME_ARGS(data->duration));

            if (current_position > 3* GST_SECOND) {
                seek_to_position(current_position - (3* GST_SECOND),data->pipeline);
            }
            gst_element_query_position(data->pipeline, GST_FORMAT_TIME, &p);
            gst_element_query_duration(data->pipeline, GST_FORMAT_TIME, &data->duration);

            g_print(" position=  % " GST_TIME_FORMAT "/ %" GST_TIME_FORMAT"\n" ,GST_TIME_ARGS(p),GST_TIME_ARGS(data->duration));

            break;

		case 'f':
				flush_pipeline(data);
				break;
				
		case 'p': 
		        data->playing = !data->playing;
                  gst_element_set_state (data->pipeline,
                data->playing ? GST_STATE_PLAYING : GST_STATE_PAUSED);
                g_print ("Setting state to %s\n", data->playing ? "PLAYING" : "PAUSE");
				break;
        default:
            cout << "Invalid input. Please enter 'f' to seek forward or 'b' to seek backward." << endl;
            break;
    }
}


bool handle_quit_input(char input,CustomData *data) {
    if (input == 'q') {
        return true;
    }
    return false;
}








int main (int argc, char * argv[])
{
  GstElement *pipe, *dec, *sink;
  GstMessage *msg;
  gchar *uri;

  gst_init (&argc, &argv);
 // GST_DEBUG_CATEGORY_INIT (my_category, "my category", 0, "This is my very own");

  string path = "/home/ee212829/Desktop/darshan/pro/"; // Replace with the actual path to your directory
     vector<string> videos;
     vector<gchar*> result;
     DIR *dir;
     struct dirent *ent;

     if ((dir = opendir(path.c_str())) != NULL) {
         while ((ent = readdir(dir)) != NULL) {
             std::string filename = ent->d_name;
             if (filename.find_last_of(".") != std::string::npos) {
                 std::string extension = filename.substr(filename.find_last_of("."));
                 if (extension == ".mp4") {
                     videos.push_back(path+filename);
                 }
             }
         }
         closedir(dir);
     }

  CustomData data;
  	GstBus *bus;
  	GstMessage *msg1;
  	GstStateChangeReturn ret1;
  	gboolean terminate = FALSE;

  	/* Initialize GStreamer */
  
	  int j=0;
  	 for(auto i:videos){
  		 cout << i << endl;
		data.source = gst_element_factory_make("filesrc", "source");
		/* Create the elements for video */

		data.decodebin = gst_element_factory_make("decodebin", "source1");
		/* Create the empty pipeline */
		data.pipeline = gst_pipeline_new("test-pipeline");

		data.vconvert = gst_element_factory_make("videoconvert", "vconvert");
		data.vsample = gst_element_factory_make("videoscale", "sample");
		data.vsink = gst_element_factory_make("autovideosink", "sink");
		if (!data.pipeline || !data.source || !data.vconvert
					|| !data.vsink) {
			g_printerr("Not all elements could be created for video.\n");
				return -1;
			}



		/* Build the pipeline. Note that we are NOT linking the source at this
		 * point. We will do it later. */
		gst_bin_add_many(GST_BIN(data.pipeline), data.source,data.decodebin, data.vconvert,
				data.vsample, data.vsink, NULL);

		  gst_element_link(data.source,data.decodebin);
		if (!gst_element_link_many(data.vconvert,data.vsample,  data.vsink, NULL)) {
			g_printerr("Elements could not be linked.\n");
			gst_object_unref(data.pipeline);
			return -1;
		}


		/* Set the URI to play */

		set_source(data.source,i);

	//	g_object_set(data.source, "location",i.c_str(),NULL);

		/* Connect to the pad-added signal */
		g_signal_connect(data.decodebin, "pad-added", G_CALLBACK (pad_added_handler),
				&data);

		/* Start playing */
		ret1 = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
		if (ret1 == GST_STATE_CHANGE_FAILURE) {
			g_printerr("Unable to set the   to the playing state.\n");
			gst_object_unref(data.pipeline);
			return -1;
		}
		data.playing = TRUE;

char input;
    while (true) {
    	cout<<"take input:";
        cin >> input;
        if (handle_quit_input(input,&data)) {
            break;
        }
        handle_seek_input(input,&data,i);
    }

 

 
       // const gchar* next_file_uri = argv[2]; // Get the URI of the next file in the list
       //flush_pipeline(data.pipeline, data.source, next_file_uri);

		/* Listen to the bus */
		bus = gst_element_get_bus(data.pipeline);
		msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS));
		//const gchar* next_file_uri = argv[2];
          
		if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
            //perform_flush(data.pipeline);
			g_print("EOS\n");
		//	gst_element_set_state(data.pipeline, GST_STATE_PAUSED);

			//g_object_set(sink, "preroll", TRUE, NULL);
         
           // flush_pipeline(data.pipeline,data.source);
           //g_signal_connect (data.pipeline, "pad-added", G_CALLBACK (flush_pipeline),data.source);

          //# Unblock the pipeline and flush the data
              //    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
			//data.pipeline.seek_simple(Gst.Format.TIME, Gst.SeekFlags.FLUSH | Gst.SeekFlags.KEY_UNIT,data.pipeline.query_duration(Gst.Format.TIME)[0],NULL);
			
			//cout<<videos[j+1]<<endl;

			
		//j++;
            
		gst_element_set_state(data.pipeline, GST_STATE_NULL);//
            //perform_flush(data.pipeline);
		}		

  	 }
  	/* Free resources */
  	gst_object_unref(bus);
  	gst_element_set_state(data.pipeline, GST_STATE_NULL);
  	gst_object_unref(data.pipeline);
    return 0;
  }
  
  
  
  
  
  