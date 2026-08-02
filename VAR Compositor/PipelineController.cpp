#include "PipelineController.h"

#include <gst/video/videooverlay.h>
#include <iostream>

PipelineController::PipelineController()
	: pipeline_(nullptr), textOverlay_(nullptr), previewSink_(nullptr), bus_(nullptr) {
}

PipelineController::~PipelineController() {
	stop();
	releaseResources();
}

std::string PipelineController::escapeForGstString(const std::string& input) {
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

GstPadProbeReturn PipelineController::loopVideoCb(GstPad* pad, GstPadProbeInfo* info, gpointer userData) {
	GstEvent* event = GST_PAD_PROBE_INFO_EVENT(info);

	if (GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
		GstElement* bgSrc = GST_ELEMENT(userData);

		gboolean success = gst_element_seek_simple(bgSrc, GST_FORMAT_TIME,
			static_cast<GstSeekFlags>(GST_SEEK_FLAG_KEY_UNIT),
			0);

		if (success) {
			std::cout << "Background video reached end. Looping..." << std::endl;
		}
		else {
			std::cerr << "Warning: Failed to loop background video." << std::endl;
		}

		return GST_PAD_PROBE_DROP;
	}

	return GST_PAD_PROBE_OK;
}

void PipelineController::configureMixerPad(GstElement* mixer, const gchar* padName,
	gint zorder, gint xpos, gint ypos, gint width, gint height) {
	GstPad* pad = gst_element_get_static_pad(mixer, padName);
	if (!pad) {
		std::cerr << "Failed to retrieve pad: " << padName << std::endl;
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

std::string PipelineController::buildPipelineDescription(const std::string& uri, const std::string& overlayText) const {
	return
		"glvideomixer name=mix background=1 ! glcolorconvert ! gldownload ! videoconvert "
		"! textoverlay name=bottom_text valignment=bottom halignment=left xpad=370 ypad=150 draw-outline=false draw-shadow=false font-desc=\"sans 14 bold\" text=\"" + overlayText + "\" "
		"! tee name=t "
		"t. ! queue ! videoconvert ! d3d11videosink name=preview_sink sync=false "
		"t. ! queue ! videoconvert ! decklinkvideosink device-number=3 mode=1080i50 "

		"uridecodebin name=bg_src uri=\"" + uri + "\" "
		"! queue leaky=downstream max-size-buffers=1 "
		"! videorate drop-only=false "
		"! videoconvert name=bg_conv "
		"! videoscale name=bg_scale ! video/x-raw,format=UYVY,width=1920,height=1080,framerate=50/1,interlace-mode=progressive "
		"! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),format=RGBA,width=1920,height=1080,framerate=50/1 ! mix.sink_0 "

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
		"glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),format=RGBA,height=1080,width=1920,framerate=50/1 ! mix.sink_3 ";
}

bool PipelineController::initialize(const std::string& templatePath, const std::string& initialOverlayText) {
	releaseResources();

	gst_init(nullptr, nullptr);

	gchar* absolutePath = g_canonicalize_filename(templatePath.c_str(), NULL);
	gchar* uri = gst_filename_to_uri(absolutePath, NULL);

	if (!uri) {
		std::cerr << "Could not convert template path to URI." << std::endl;
		g_free(absolutePath);
		return false;
	}

	const std::string pipelineDesc = buildPipelineDescription(std::string(uri), escapeForGstString(initialOverlayText));

	GError* error = nullptr;
	pipeline_ = gst_parse_launch(pipelineDesc.c_str(), &error);

	g_free(absolutePath);
	g_free(uri);

	if (error) {
		std::cerr << "Pipeline parsing error: " << error->message << std::endl;
		g_clear_error(&error);
		releaseResources();
		return false;
	}

	GstElement* bgSrc = gst_bin_get_by_name(GST_BIN(pipeline_), "bg_src");
	GstElement* bgScale = gst_bin_get_by_name(GST_BIN(pipeline_), "bg_scale");
	GstElement* mixer = gst_bin_get_by_name(GST_BIN(pipeline_), "mix");
	textOverlay_ = gst_bin_get_by_name(GST_BIN(pipeline_), "bottom_text");
	previewSink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "preview_sink");

	if (!bgSrc || !bgScale || !mixer || !textOverlay_ || !previewSink_) {
		std::cerr << "Could not retrieve one or more required pipeline elements." << std::endl;
		if (bgSrc) {
			gst_object_unref(bgSrc);
		}
		if (bgScale) {
			gst_object_unref(bgScale);
		}
		if (mixer) {
			gst_object_unref(mixer);
		}
		releaseResources();
		return false;
	}

	GstPad* scaleSinkPad = gst_element_get_static_pad(bgScale, "sink");
	gst_pad_add_probe(scaleSinkPad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, loopVideoCb, bgSrc, NULL);
	gst_object_unref(scaleSinkPad);

	configureMixerPad(mixer, "sink_0", 0, 0, 0, 1920, 1080);
	configureMixerPad(mixer, "sink_1", 0, 28, 60, 1250, 702);
	configureMixerPad(mixer, "sink_2", 1, 1296, 60, 600, 335);
	configureMixerPad(mixer, "sink_3", 1, 1296, 426, 600, 335);

	bus_ = gst_element_get_bus(pipeline_);

	gst_object_unref(bgSrc);
	gst_object_unref(bgScale);
	gst_object_unref(mixer);

	return true;
}

bool PipelineController::start() {
	if (!pipeline_) {
		return false;
	}

	const GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		std::cerr << "Failed to set pipeline to PLAYING state." << std::endl;
		return false;
	}

	return true;
}

void PipelineController::stop() {
	if (pipeline_) {
		gst_element_set_state(pipeline_, GST_STATE_NULL);
	}
}

void PipelineController::setOverlayText(const std::string& text) {
	if (!textOverlay_) {
		return;
	}

	g_object_set(textOverlay_, "text", text.c_str(), NULL);
}

void PipelineController::setPreviewWindowHandle(std::uintptr_t windowHandle) {
	if (!previewSink_ || !GST_IS_VIDEO_OVERLAY(previewSink_)) {
		return;
	}

	gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(previewSink_), static_cast<guintptr>(windowHandle));
	gst_video_overlay_handle_events(GST_VIDEO_OVERLAY(previewSink_), TRUE);
}

bool PipelineController::processBusMessages() {
	if (!bus_) {
		return true;
	}

	GstMessage* msg = nullptr;
	while ((msg = gst_bus_pop_filtered(bus_, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS))) != nullptr) {
		switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_ERROR: {
			GError* err = nullptr;
			gchar* debugInfo = nullptr;
			gst_message_parse_error(msg, &err, &debugInfo);
			std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src)
				<< ": " << err->message << std::endl;
			std::cerr << "Debugging information: " << (debugInfo ? debugInfo : "none") << std::endl;
			g_clear_error(&err);
			g_free(debugInfo);
			gst_message_unref(msg);
			return false;
		}
		case GST_MESSAGE_EOS:
			std::cout << "End-Of-Stream reached." << std::endl;
			gst_message_unref(msg);
			return false;
		default:
			break;
		}

		gst_message_unref(msg);
	}

	return true;
}

void PipelineController::releaseResources() {
	if (bus_) {
		gst_object_unref(bus_);
		bus_ = nullptr;
	}

	if (textOverlay_) {
		gst_object_unref(textOverlay_);
		textOverlay_ = nullptr;
	}

	if (previewSink_) {
		gst_object_unref(previewSink_);
		previewSink_ = nullptr;
	}

	if (pipeline_) {
		gst_object_unref(pipeline_);
		pipeline_ = nullptr;
	}
}
