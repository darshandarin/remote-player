#include<gst/gst.h>

int main(int argv,char* argc)
{

    GstElement *pipeline;
    GstMessage *msg;
    GstBus *bus;

    gst_init(&argv,&argc);

    pipeline=gst_parse_launch("playbin uri=file:///home/ee212829/Desktop/playlist/new1.mp4",NULL);

    gst_element_set_state(pipeline,GST_STATE_PLAYING);

    bus=gst_element_get_bus(pipeline);
    msg=gst_bus_timed_pop_filtered(bus,GST_CLOCK_TIME_NONE,GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    if(GST_MESSAGE_TYPE(msg)==GST_MESSAGE_ERROR)
    {
        g_printerr("there is an error in the pipeline");
    }

    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline,GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;


}