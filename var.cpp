#include <gst/gst.h>
#include <dirent.h>
#include <iostream>
#include <random>
#include <map>

static void on_new_pad (GstElement * dec, GstPad * pad, GstElement * fakesink) {
    GstPad *sinkpad;
    sinkpad = gst_element_get_static_pad (fakesink, "sink");
    if (!gst_pad_is_linked (sinkpad)) {
        if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK)
            g_error ("Failed to link pads!");
    }
    gst_object_unref (sinkpad);
}

std::multimap<std::string, std::string> finding_tags (int argc, char **argv) {

    GstElement *pipe, *dec, *sink;
    GstMessage *msg;
    gchar *uri;

    DIR *myDirectory;
    struct dirent *myFile;
    char buf;

    std::multimap<std::string ,std::string> file_paths;

    gst_init (&argc, &argv);

    if (argc < 2)
        g_error ("Usage: %s FILE or URI", argv[0]);

    myDirectory = opendir(argv[1]);
    if(myDirectory) {

        while (myFile = readdir(myDirectory)) {
            std::string path;
            path = "file:///home/ee212829/Videos";
            path = path + argv[1] + "/";
            path += myFile->d_name;

            if (myFile->d_type == DT_REG)  {

              if (gst_uri_is_valid (path.c_str())) {
                uri = g_strdup (path.c_str());
              }
              else {
                uri = gst_filename_to_uri (path.c_str(), NULL);
              }

              pipe = gst_pipeline_new ("pipeline");

              dec = gst_element_factory_make ("uridecodebin", NULL);
              g_object_set (dec, "uri", uri, NULL);
              gst_bin_add (GST_BIN (pipe), dec);

              sink = gst_element_factory_make ("fakesink", NULL);
              gst_bin_add (GST_BIN (pipe), sink);

              g_signal_connect (dec, "pad-added", G_CALLBACK (on_new_pad), sink);

              gst_element_set_state (pipe, GST_STATE_PAUSED);

              while (TRUE)
              {
                  GstTagList *tags = NULL;

                  msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (pipe),
                      GST_CLOCK_TIME_NONE,
                      (GstMessageType)(GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_TAG | GST_MESSAGE_ERROR));

                  if (GST_MESSAGE_TYPE (msg) != GST_MESSAGE_TAG) /* error or async_done */
                      break;

                  gst_message_parse_tag (msg, &tags);

                  gchar *artist;

                  if (gst_tag_list_get_string(tags, GST_TAG_ARTIST, &artist)) {
                      // g_print("Artist : %s\n", artist);

                      file_paths.insert(std::pair<std::string, std::string> (artist, path));

                      g_free(artist);

                  }

                  gst_tag_list_unref (tags);

                  gst_message_unref (msg);
              }

              if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
                  GError *err = NULL;

                  gst_message_parse_error (msg, &err, NULL);
                  g_printerr ("Got error: %s\n", err->message);
                  g_error_free (err);
              }
              gst_message_unref (msg);
              gst_element_set_state (pipe, GST_STATE_NULL);
              gst_object_unref (pipe);
              g_free (uri);
            }
        }
    }
    else {
      g_printerr("failed in open a directory\n");
    }
    closedir(myDirectory);

    return file_paths;
}

int main(int argc, char **argv) {
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    std::multimap <std::string, std::string> mp4_files;
    mp4_files = finding_tags(argc, argv);

    // for (auto i : mp4_files) {
    //     std::cout << i.first << " " << i.second <<std::endl;
    // }

    // Convert the multimap into a vector of pairs
    std::vector<std::pair<std::string, std::string>> random_files(mp4_files.begin(), mp4_files.end());

    pipeline = gst_element_factory_make("playbin", "pipeline");

    if(!pipeline) {
        g_printerr("filed to create a pipeline\n");
    }

    while (true) {

        // Generate a random index
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, random_files.size() - 1);
        int index = dis(gen);

        // Get the value at the random index
        std::string file = random_files[index].second;


        // Print the result
        std::cout << "Randomly selected file : " << file << std::endl;

        g_object_set(G_OBJECT(pipeline), "uri", file.c_str(), NULL);

        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        bus = gst_element_get_bus(pipeline);
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

        if(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
            g_print("End of stremam reached\n");
            gst_element_set_state(pipeline, GST_STATE_NULL);
            g_print("\n*******************\n\n");
        }

        if(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError *err;
            gchar *debug;
            gst_message_parse_error(msg, &err, &debug);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debug information %s\n : ", debug ? debug : "none");
            g_clear_error(&err);
            g_free(debug);
            break;
        }
    }
    gst_object_unref(pipeline);
    gst_object_unref(bus);
    gst_message_unref(msg);
    return 0;
}
#include <gst/gst.h>
#include <dirent.h>
#include <iostream>
#include <random>
#include <map>

static void on_new_pad (GstElement * dec, GstPad * pad, GstElement * fakesink) {
    GstPad *sinkpad;
    sinkpad = gst_element_get_static_pad (fakesink, "sink");
    if (!gst_pad_is_linked (sinkpad)) {
        if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK)
            g_error ("Failed to link pads!");
    }
    gst_object_unref (sinkpad);
}

std::multimap<std::string, std::string> finding_tags (int argc, char **argv) {

    GstElement *pipe, *dec, *sink;
    GstMessage *msg;
    gchar *uri;

    DIR *myDirectory;
    struct dirent *myFile;
    char buf;

    std::multimap<std::string ,std::string> file_paths;

    gst_init (&argc, &argv);

    if (argc < 2)
        g_error ("Usage: %s FILE or URI", argv[0]);

    myDirectory = opendir(argv[1]);
    if(myDirectory) {

        while (myFile = readdir(myDirectory)) {
            std::string path;
            path = "file:///home/ee212829/Videos";
            path = path + argv[1] + "/";
            path += myFile->d_name;

            if (myFile->d_type == DT_REG)  {

              if (gst_uri_is_valid (path.c_str())) {
                uri = g_strdup (path.c_str());
              }
              else {
                uri = gst_filename_to_uri (path.c_str(), NULL);
              }

              pipe = gst_pipeline_new ("pipeline");

              dec = gst_element_factory_make ("uridecodebin", NULL);
              g_object_set (dec, "uri", uri, NULL);
              gst_bin_add (GST_BIN (pipe), dec);

              sink = gst_element_factory_make ("fakesink", NULL);
              gst_bin_add (GST_BIN (pipe), sink);

              g_signal_connect (dec, "pad-added", G_CALLBACK (on_new_pad), sink);

              gst_element_set_state (pipe, GST_STATE_PAUSED);

              while (TRUE)
              {
                  GstTagList *tags = NULL;

                  msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (pipe),
                      GST_CLOCK_TIME_NONE,
                      (GstMessageType)(GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_TAG | GST_MESSAGE_ERROR));

                  if (GST_MESSAGE_TYPE (msg) != GST_MESSAGE_TAG) /* error or async_done */
                      break;

                  gst_message_parse_tag (msg, &tags);

                  gchar *artist;

                  if (gst_tag_list_get_string(tags, GST_TAG_ARTIST, &artist)) {
                      // g_print("Artist : %s\n", artist);

                      file_paths.insert(std::pair<std::string, std::string> (artist, path));

                      g_free(artist);

                  }

                  gst_tag_list_unref (tags);

                  gst_message_unref (msg);
              }

              if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
                  GError *err = NULL;

                  gst_message_parse_error (msg, &err, NULL);
                  g_printerr ("Got error: %s\n", err->message);
                  g_error_free (err);
              }
              gst_message_unref (msg);
              gst_element_set_state (pipe, GST_STATE_NULL);
              gst_object_unref (pipe);
              g_free (uri);
            }
        }
    }
    else {
      g_printerr("failed in open a directory\n");
    }
    closedir(myDirectory);

    return file_paths;
}

int main(int argc, char **argv) {
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    std::multimap <std::string, std::string> mp4_files;
    mp4_files = finding_tags(argc, argv);

    // for (auto i : mp4_files) {
    //     std::cout << i.first << " " << i.second <<std::endl;
    // }

    // Convert the multimap into a vector of pairs
    std::vector<std::pair<std::string, std::string>> random_files(mp4_files.begin(), mp4_files.end());

    pipeline = gst_element_factory_make("playbin", "pipeline");

    if(!pipeline) {
        g_printerr("filed to create a pipeline\n");
    }

    while (true) {

        // Generate a random index
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, random_files.size() - 1);
        int index = dis(gen);

        // Get the value at the random index
        std::string file = random_files[index].second;


        // Print the result
        std::cout << "Randomly selected file : " << file << std::endl;

        g_object_set(G_OBJECT(pipeline), "uri", file.c_str(), NULL);

        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        bus = gst_element_get_bus(pipeline);
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

        if(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
            g_print("End of stremam reached\n");
            gst_element_set_state(pipeline, GST_STATE_NULL);
            g_print("\n*******************\n\n");
        }

        if(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError *err;
            gchar *debug;
            gst_message_parse_error(msg, &err, &debug);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debug information %s\n : ", debug ? debug : "none");
            g_clear_error(&err);
            g_free(debug);
            break;
        }
    }
    gst_object_unref(pipeline);
    gst_object_unref(bus);
    gst_message_unref(msg);
    return 0;
}
