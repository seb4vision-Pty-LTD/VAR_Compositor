#pragma once

#include <gst/gst.h>
#include <cstdint>
#include <string>

enum class PipelineMode {
	Var,
	Program
};

class PipelineController {
public:
	PipelineController();
	~PipelineController();

	bool initialize(const std::string& templatePath, const std::string& initialOverlayText, const std::string& templateBugPath= "C:/Temp/VAR_BUG.mp4");
	bool start();
	void stop();
	bool setMode(PipelineMode mode);
	PipelineMode mode() const;

	void setOverlayText(const std::string& text);
	void setPreviewWindowHandle(std::uintptr_t windowHandle);

	bool processBusMessages();

private:
	GstElement* pipeline_;
	GstElement* textOverlay_;
	GstElement* previewSink_;
	GstBus* bus_;
	std::string templateUri_;
	std::string templateBugUri_;
	std::string overlayText_;
	std::uintptr_t previewWindowHandle_;
	PipelineMode activeMode_;
	bool isRunning_;

	static std::string escapeForGstString(const std::string& input);
	static GstPadProbeReturn loopVideoCb(GstPad* pad, GstPadProbeInfo* info, gpointer userData);
	static void configureMixerPad(GstElement* mixer, const gchar* padName,
		gint zorder, gint xpos, gint ypos, gint width, gint height);

	bool rebuildActivePipeline();
	std::string buildVarPipelineDescription(const std::string& uri, const std::string& overlayText) const;
	std::string buildProgramPipelineDescription(const std::string& uri, const std::string& overlayText) const;
	void releaseResources();
};
