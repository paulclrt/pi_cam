#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

static void stream_starting(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data) {
    g_print("Stream started\n");
}

int main(int argc, char *argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create an RTSP server instance
    GstRTSPServer *server = gst_rtsp_server_new();
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    // Set up the pipeline for video capture and streaming
    // - v4l2src: video capture from /dev/video1
    // - videoconvert: format conversion
    // - x264enc: video encoding (you can change this to vp8enc or others)
    // - rtph264pay: RTP payload for H264 encoding
    // - udpsink: send the RTP stream (this will eventually be wrapped by the RTSP server)
    const gchar *pipeline_str = 
        "v4l2src device=/dev/video1 ! "
        "videoconvert ! "
        "x264enc tune=zerolatency ! "
        "rtph264pay name=pay0 pt=96";

    GstElement *pipeline = gst_parse_launch(pipeline_str, NULL);
    gst_rtsp_media_factory_set_launch(factory, pipeline_str);

    // Attach the factory to the server on a specific mount point (e.g., /test)
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);

    // Connect signal for media starting (optional)
    g_signal_connect(factory, "media-configure", G_CALLBACK(stream_starting), NULL);

    // Start the RTSP server
    gst_rtsp_server_attach(server, NULL);

    // Print the RTSP URL to connect to (e.g., rtsp://localhost:8554/test)
    g_print("Stream ready at rtsp://localhost:8554/test\n");

    // Start the GMainLoop
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // Clean up (though this might never be reached unless the loop exits)
    g_main_loop_unref(loop);
    gst_object_unref(server);

    return 0;
}

