#include "crow.h"
#include "crow/json.h"


#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


#include <string.h>
#include <fstream>
#include <sstream>


// this is obviously not secure. But this is a learnign project so idc
#define USERNAME "admin"
#define PASSWORD "superadmin"
std::unordered_map<std::string, std::string> sessions;



#define VIDEO_DEVICE "/dev/video0"

void start_video_capture() {
    cap.open(VIDEO_DEVICE, cv::CAP_V4L2); // Open the camera device
    if (!cap.isOpened()) {
        std::cerr << "Error opening video stream!" << std::endl;
        exit(1);
    }
}


void stream_mjpeg(crow::response& res) {
    res.set_header("Content-Type", "multipart/x-mixed-replace; boundary=frame");

    // Loop to send video frames as MJPEG
    while (true) {
        cv::Mat frame;
        cap >> frame;  // Capture a new frame

        if (frame.empty()) {
            break;
        }

        // Encode frame to JPEG
        std::vector<uchar> buf;
        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 90};
        cv::imencode(".jpg", frame, buf, params);

        // Write MJPEG frame
        res.write("--frame\r\n");
        res.write("Content-Type: image/jpeg\r\n");
        res.write("Content-Length: " + std::to_string(buf.size()) + "\r\n\r\n");
        res.write(buf.data(), buf.size());
        res.write("\r\n");
        
        usleep(30000);  // Sleep to simulate 30fps streaming
    }
}





int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/").methods("GET"_method)([]() {
        std::ifstream file("static/home_login.html");
        if (!file) {
            return crow::response(404, "File not found");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string html_content = buffer.str();

        return crow::response(html_content);
    });

    CROW_ROUTE(app, "/login").methods("POST"_method)([](const crow::request& req) {
            auto json = crow::json::load(req.body);
            if (!json) {
                return crow::response(400, "Invalid json");
            }

            std::string username = json["username"].s();
            std::string password = json["password"].s();

            if (username == USERNAME && password == PASSWORD) {
                std::string session_id = "session_" + std::to_string(sessions.size() + 1);
                sessions[session_id] = username;

                crow::json::wvalue response;
                response["status"] = "success";
                response["session_id"] = session_id;
                

                // that cookie is so easy to guess. idc about security here. I just want to learn to crosscompile and add my program to a custom linuximage
                crow::response res(response);
                res.add_header("Set-Cookie", "session_id=" + session_id + "; Path=/");

                return res;

            } else {

                return crow::response(401, "Invalid Credentials");
            }

    });

    // CROW_ROUTE(app, "/logout").methods("DELETE"_method)([](const crow::request& req) {
    //         auto json = crow::json::load(req.body);
    //         if (!json) {
    //             return crow::response(400, "Invalid json");
    //         }
    //
    //         std::string session_id = json["session_id"].s();
    //
    //         sessions.erase(session_id);
    //
    //         crow::json::wvalue response;
    //         response["status"] = "success";
    //         response["session_id"] = session_id;
    //
    //         return crow::response(response);
    // });


    CROW_ROUTE(app, "/video")([&sessions](const crow::request& req) {
        // Get the session ID from the cookie
        auto cookies = req.get_header_value("Cookie");
        std::string session_id;

        // Validate the session
        size_t pos = cookies.find("session_id=");
        if (pos != std::string::npos) {
            size_t end = cookies.find(";", pos);
            session_id = cookies.substr(pos + 11, end - (pos + 11));
        }
        if (sessions.find(session_id) != sessions.end()) {
            // return crow::response.set_static_file_info("videoplayer.html");
            return crow::response(200, "Welcome to the video page!");            
        } else {
            return crow::response(403, "Access denied");
        }
    });
    


    CROW_ROUTE(app, "/stream")([&sessions](const crow::request& req) {
        auto cookies = req.get_header_value("Cookie");
        std::string session_id;

        // Validate the session
        size_t pos = cookies.find("session_id=");
        if (pos != std::string::npos) {
            size_t end = cookies.find(";", pos);
            session_id = cookies.substr(pos + 11, end - (pos + 11));
        }
        if (sessions.find(session_id) != sessions.end()) {
            stream_mjpeg(res);
        } else {
            return crow::response(403, "Access denied");
        }
    });

    app.port(8080).multithreaded().run();
    return 0;
}
