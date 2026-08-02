#pragma once

#include <gst/gst.h>
#include <cstdint>
#include <string>

class PipelineController {
public:
	PipelineController();
	~PipelineController();

	bool initialize(const std::string& templatePath, const std::string& initialOverlayText);
	bool start();
	void stop();

	void setOverlayText(const std::string& text);
	void setPreviewWindowHandle(std::uintptr_t windowHandle);

	bool processBusMessages();

private:
	GstElement* pipeline_;
	GstElement* textOverlay_;
	GstElement* previewSink_;
	GstBus* bus_;

	static std::string escapeForGstString(const std::string& input);
	static GstPadProbeReturn loopVideoCb(GstPad* pad, GstPadProbeInfo* info, gpointer userData);
	static void configureMixerPad(GstElement* mixer, const gchar* padName,
		gint zorder, gint xpos, gint ypos, gint width, gint height);

	std::string buildPipelineDescription(const std::string& uri, const std::string& overlayText) const;
	void releaseResources();
};
