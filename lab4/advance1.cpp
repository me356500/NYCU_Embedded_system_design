#pragma GCC optimize("unroll-loops")
#include <fcntl.h> 
#include <fstream>
#include <linux/fb.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <sys/ioctl.h>
#include <unistd.h>
#include <opencv2/core/core_c.h>

#include <mutex>
#include <thread>
#include <string>
#include <iostream>
#include <termios.h>

using namespace std;


// https://elixir.bootlin.com/linux/latest/source/include/linux/fb.h
// line 242 fb_var_screeninfo

// https://www.kernel.org/doc/html/v4.15/driver-api/frame-buffer.html

struct framebuffer_info
{
    uint32_t bits_per_pixel;    // framebuffer depth
    uint32_t xres_virtual;      // how many pixel in a row in virtual screen
    uint32_t yres_virtual;      // how many pixel in a column in virtual screen
};

struct framebuffer_info get_framebuffer_info(const char* framebuffer_device_path) {
    struct framebuffer_info info;
    struct fb_var_screeninfo screen_info;

    // open devices, read and write
    // file descriptor
    int fd = open(framebuffer_device_path, O_RDWR);
    
    if (fd == -1) {
        cerr << "Cannot open : " << framebuffer_device_path << "\n";
    }

    /* ioctl (int fd, request, ...)

        FBIOGET_VSCREENINFO：get framebuffer var info
        FBIOPUT_VSCREENINFO：set framebuffer var info
        FBIOGET_FSCREENINFO：get framebuffer fix info
    
    */
    else if (!ioctl(fd, FBIOGET_VSCREENINFO, &screen_info)) {
        info.xres_virtual = screen_info.xres_virtual;
        info.yres_virtual = screen_info.yres_virtual;
        info.bits_per_pixel = screen_info.bits_per_pixel;
    }
    
    return info;
};



framebuffer_info fb_info;
cv::Mat image, frame;
cv::Size2f image_size;


int main(int argc, char **argv) {

    // get info of the framebuffer
    fb_info = get_framebuffer_info("/dev/fb0");
    std::ofstream ofs("/dev/fb0");


    // 3840 x 1080
    image = cv::imread("picture.png", cv::IMREAD_UNCHANGED);
    cv::cvtColor(image, image, cv::COLOR_BGRA2BGR565);
    
    vector<cv::Mat> scrollboard(2, image);
    /*
        |        |        |
        |  IMG1  |  IMG2  | 
        |        |        |
        0      3839      7679
        horizontal concat
        mod 3840
    */
    cv::hconcat(scrollboard, frame);
    
    int shift = 0;
    int step = atoi(argv[1]);

    while (1) {

        shift = (shift + step) % 3840;

        for (int i = 0; i < 1080; ++i) {
            
            ofs.seekp(i * 2 * fb_info.xres_virtual);
            ofs.write(reinterpret_cast<char*>(frame.ptr(i, shift)), 2 * 1920);
        }
    
    }

    return 0;
}
