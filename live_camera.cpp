#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <SDL2/SDL.h>

#define WIDTH  640
#define HEIGHT 480
#define DEVICE "/dev/video0"

// Function to convert YUYV to RGB for SDL2
void YUYVtoRGB(uint8_t *yuyv, uint8_t *rgb, int width, int height) {
    int i, j;
    for (i = 0, j = 0; i < width * height * 2; i += 4, j += 6) {
        int Y1 = yuyv[i];
        int U  = yuyv[i + 1] - 128;
        int Y2 = yuyv[i + 2];
        int V  = yuyv[i + 3] - 128;

        int C1 = Y1 - 16, C2 = Y2 - 16;
        int D = U, E = V;

        rgb[j]     = std::max(0, std::min(255, (298 * C1 + 409 * E + 128) >> 8));  // R1
        rgb[j + 1] = std::max(0, std::min(255, (298 * C1 - 100 * D - 208 * E + 128) >> 8));  // G1
        rgb[j + 2] = std::max(0, std::min(255, (298 * C1 + 516 * D + 128) >> 8));  // B1
        rgb[j + 3] = std::max(0, std::min(255, (298 * C2 + 409 * E + 128) >> 8));  // R2
        rgb[j + 4] = std::max(0, std::min(255, (298 * C2 - 100 * D - 208 * E + 128) >> 8));  // G2
        rgb[j + 5] = std::max(0, std::min(255, (298 * C2 + 516 * D + 128) >> 8));  // B2
    }
}

int main() {
    // 1️⃣ Open the camera device
    int fd = open(DEVICE, O_RDWR);
    if (fd == -1) {
        std::cerr << "Failed to open " << DEVICE << std::endl;
        return -1;
    }

    // 2️⃣ Set video format
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        std::cerr << "Failed to set format" << std::endl;
        close(fd);
        return -1;
    }

    // 3️⃣ Request buffer
    struct v4l2_requestbuffers req;
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        std::cerr << "Failed to request buffer" << std::endl;
        close(fd);
        return -1;
    }

    // 4️⃣ Get buffer
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
        std::cerr << "Failed to query buffer" << std::endl;
        close(fd);
        return -1;
    }

    // 5️⃣ Map buffer
    uint8_t *buffer = (uint8_t *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer == MAP_FAILED) {
        std::cerr << "Failed to mmap buffer" << std::endl;
        close(fd);
        return -1;
    }

    // 6️⃣ Start video streaming
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        std::cerr << "Failed to start streaming" << std::endl;
        close(fd);
        return -1;
    }

    // 7️⃣ Initialize SDL2 for rendering
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Initialization failed" << std::endl;
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("Camera Feed", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "SDL Window creation failed" << std::endl;
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    
    uint8_t *rgb_buffer = new uint8_t[WIDTH * HEIGHT * 3];

    bool running = true;
    SDL_Event event;
    
    while (running) {
        // 8️⃣ Capture frame
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            std::cerr << "Failed to queue buffer" << std::endl;
            break;
        }

        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            std::cerr << "Failed to dequeue buffer" << std::endl;
            break;
        }

        // 🔹 Convert YUYV to RGB
        YUYVtoRGB(buffer, rgb_buffer, WIDTH, HEIGHT);

        // 🔹 Update SDL texture
        SDL_UpdateTexture(texture, NULL, rgb_buffer, WIDTH * 3);

        // 🔹 Render frame
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // 🔹 Check for quit event
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
    }

    // 🔚 Cleanup
    delete[] rgb_buffer;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    ioctl(fd, VIDIOC_STREAMOFF, &type);
    munmap(buffer, buf.length);
    close(fd);

    return 0;
}

