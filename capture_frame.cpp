#include <fcntl.h>      // open()
#include <unistd.h>     // close()
#include <sys/ioctl.h>  // ioctl()
#include <linux/videodev2.h> // V4L2 driver interface
#include <sys/mman.h>   // mmap()
#include <iostream>
#include <cstring>
#include <errno.h>

#define DEVICE "/dev/video0"  // Camera device file

int main() {
    // Open the camera device
    int fd = open(DEVICE, O_RDWR);
    if (fd == -1) {
        std::cerr << "Failed to open " << DEVICE << ": " << strerror(errno) << std::endl;
        return -1;
    }

    // Query device capabilitie 
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        std::cerr << "Failed to get device capabilities: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }
    std::cout << "Camera: " << cap.card << ", Driver: " << cap.driver << std::endl;

    // Set video format (resolution & pixel format)
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;    // Set width
    fmt.fmt.pix.height = 480;   // Set height
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // Use YUYV format
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        std::cerr << "Failed to set format: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    // Request buffer memory from the kernel
    struct v4l2_requestbuffers req;
    req.count = 1; // Ask for 1 buffer
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        std::cerr << "Failed to request buffer: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    // Get buffer info
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
        std::cerr << "Failed to query buffer: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    // Map memory
    void *buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer == MAP_FAILED) {
        std::cerr << "Failed to mmap buffer: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    // Queue buffer for capturing
    if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
        std::cerr << "Failed to queue buffer: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    // Start capturing video
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        std::cerr << "Failed to start stream: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    // Dequeue buffer to get the captured frame
    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        std::cerr << "Failed to dequeue buffer: " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    // Save raw frame to a file
    FILE *file = fopen("frame.raw", "wb");
    if (file) {
        fwrite(buffer, buf.bytesused, 1, file);
        fclose(file);
        std::cout << "Frame captured and saved to frame.raw" << std::endl;
    } else {
        std::cerr << "Failed to save frame to file" << std::endl;
    }

    // Stop streaming & clean up
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    munmap(buffer, buf.length);
    close(fd);

    return 0;
}

