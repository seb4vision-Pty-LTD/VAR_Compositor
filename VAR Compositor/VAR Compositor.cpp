#include <gst/gst.h>
#include <iostream>
#include <string>
#include <filesystem>

std::string escape_for_gst_string(const std::string& input) {
    std::string escaped;
    escaped.reserve(input.size());

    for (char c : input) {
        if (c == '\\' || c == '"') {
            escaped.push_back('\\');
        }
        escaped.push_back(c);
    }

    return escaped;
}


// 1. Define the Pad Probe Callback to handle the loop
static GstPadProbeReturn loop_video_cb(GstPad* pad, GstPadProbeInfo* info, gpointer user_data) {
    GstEvent* event = GST_PAD_PROBE_INFO_EVENT(info);

    // Check if the event flowing through the pad is an End-Of-Stream (EOS)
    if (GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
        GstElement* bg_src = GST_ELEMENT(user_data);

        // Issue a flushing seek to rewind the source element to 0 nanoseconds
        gboolean success = gst_element_seek_simple(bg_src, GST_FORMAT_TIME,
            static_cast<GstSeekFlags>( GST_SEEK_FLAG_KEY_UNIT),
             0);

        if (success) {
            std::cout << "Background video reached end. Looping..." << std::endl;
        }
        else {
            std::cerr << "Warning: Failed to loop background video." << std::endl;
        }

        // Drop the EOS event so the glvideomixer never receives it
        return GST_PAD_PROBE_DROP;
    }

    // For all other events (buffers, timestamps, etc.), let them pass normally
    return GST_PAD_PROBE_OK;
}


// Helper function to configure a glvideomixer pad
void configure_mixer_pad(GstElement* mixer, const gchar* pad_name,
    gint zorder, gint xpos, gint ypos, gint width, gint height) {
    GstPad* pad = gst_element_get_static_pad(mixer, pad_name);
    if (!pad) {
        std::cerr << "Failed to retrieve pad: " << pad_name << std::endl;
        return;
    }

    g_object_set(pad,
        "zorder", zorder,
        "xpos", xpos,
        "ypos", ypos,
        "width", width,
        "height", height,
        NULL);
    gst_object_unref(pad);
}

int main(int argc, char* argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // uridecodebin requires a valid URI format.
    // For local Windows files, use file:/// followed by the absolute path with forward slashes.
    std::string template_uri = "C:/Temp/VarClear.mp4";
    std::string overlay_text = "CUSTOM TEXT";
    std::string overlay_text_escaped = escape_for_gst_string(overlay_text);

    gchar* absolute_path = g_canonicalize_filename(template_uri.c_str(), NULL);
    gchar* uri = gst_filename_to_uri(absolute_path, NULL);

    // Build the pipeline string replacing filesrc/decodebin with uridecodebin
    std::string pipeline_desc =
        "glvideomixer name=mix background=1 ! glcolorconvert ! gldownload ! videoconvert "
        "! textoverlay name=bottom_text valignment=bottom halignment=center ypad=24 draw-shadow=true font-desc=\"Sans 36\" text=\"" + overlay_text_escaped + "\" "
        "! tee name=t "
        "t. ! queue ! videoconvert ! autovideosink "
        "t. ! queue ! videoconvert ! video/x-raw,format=UYVY,width=1920,height=1080,framerate=50/1,interlace-mode=progressive ! decklinkvideosink device-number=3 mode=1080i50 "
        
		// file source for the background template video
        "uridecodebin name=bg_src uri=\"" + std::string(uri) + "\" "
        "! queue leaky=downstream max-size-buffers=1 "
		"! videorate drop-only=false "
        "! videoconvert name=bg_conv "
        "! videoscale name=bg_scale ! video/x-raw,format=UYVY,width=1920,height=1080,framerate=50/1,interlace-mode=progressive "
        "! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),format=RGBA,width=1920,height=1080,framerate=50/1 ! mix.sink_0 "
        
		// Live Input sources from DeckLink devices
        "decklinkvideosrc device-number=0 mode=1080i50 ! "
        "video/x-raw,format=UYVY,width=1920,height=1080 ! "
        "deinterlace mode=interlaced method=linear ! "
		"videoconvert ! "
        "video/x-raw,format=UYVY,width=1920,height=1080,framerate=50/1,interlace-mode=progressive ! "
        "glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),format=RGBA,height=1080,width=1920,framerate=50/1 ! mix.sink_1 "

        "decklinkvideosrc device-number=1 mode=1080i50 ! "
        "video/x-raw,format=UYVY,width=1920,height=1080 ! "
        "deinterlace mode=interlaced method=linear ! "
		"videoconvert ! "
        "video/x-raw,format=UYVY,width=1920,height=1080,framerate=50/1,interlace-mode=progressive ! "
        "glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),format=RGBA,height=1080,width=1920,framerate=50/1 ! mix.sink_2 "

        "decklinkvideosrc device-number=2 mode=1080i50 ! "
        "video/x-raw,format=UYVY,width=1920,height=1080 ! "
        "deinterlace mode=interlaced method=linear ! "
		"videoconvert ! "
        "video/x-raw,format=UYVY,width=1920,height=1080,framerate=50/1,interlace-mode=progressive ! "
        "glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),format=RGBA,height=1080,width=1920,framerate=50/1 ! mix.sink_3 "


        //"decklinkvideosrc device-number=1 mode=1080i50 ! glupload ! glcolorconvert ! mix.sink_2 "
        //"decklinkvideosrc device-number=2 mode=1080i50 ! glupload ! glcolorconvert ! mix.sink_3"
        ;

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(pipeline_desc.c_str(), &error);



    if (error) {
        std::cerr << "Pipeline parsing error: " << error->message << std::endl;
        g_clear_error(&error);
        return -1;
    }

    g_free(absolute_path);
    g_free(uri);

    GstElement* bg_src = gst_bin_get_by_name(GST_BIN(pipeline), "bg_src");
    GstElement* bg_conv = gst_bin_get_by_name(GST_BIN(pipeline), "bg_scale");

    if (bg_src && bg_conv) {
        GstPad* conv_sink_pad = gst_element_get_static_pad(bg_conv, "sink");
        // Attach our loop_video_cb function to the pad
        gst_pad_add_probe(conv_sink_pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
            loop_video_cb, bg_src, NULL);
        gst_object_unref(conv_sink_pad);
        gst_object_unref(bg_conv);
    }
    else {
        std::cerr << "Warning: Could not setup loop. Elements not found." << std::endl;
    }

    // Retrieve the mixer element to configure spatial properties
    GstElement* mixer = gst_bin_get_by_name(GST_BIN(pipeline), "mix");
    if (!mixer) {
        std::cerr << "Could not find glvideomixer in the pipeline." << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Configure the background template pad (zorder 0 so it stays at the back)
    configure_mixer_pad(mixer, "sink_0", 0, 0, 0, 1920, 1080);

    // Configure Device 0: Large panel (left)
    //configure_mixer_pad(mixer, "sink_1", 1, 28, 60, 1260, 708);
    configure_mixer_pad(mixer, "sink_1", 0, 28, 60, 1250, 702);

    // Configure Device 1: Upper right panel
    configure_mixer_pad(mixer, "sink_2", 1, 1296, 60, 600, 335);

    // Configure Device 2: Bottom-right panel
    configure_mixer_pad(mixer, "sink_3", 1, 1296, 426, 600, 335);

    gst_object_unref(mixer);

    // Start playing
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to PLAYING state." << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    std::cout << "Running compositor. Rendering to Device 3 and AutoVideoSink..." << std::endl;
    std::cout << "Using URI: " << template_uri << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;

    // Listen to the bus
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Handle messages
    if (msg != nullptr) {
        GError* err;
        gchar* debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src)
                << ": " << err->message << std::endl;
            std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
            g_clear_error(&err);
            g_free(debug_info);
            break;
        case GST_MESSAGE_EOS:
            std::cout << "End-Of-Stream reached." << std::endl;
            break;
        default:
            std::cerr << "Unexpected message received." << std::endl;
            break;
        }
        gst_message_unref(msg);
    }

    // Clean up
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}