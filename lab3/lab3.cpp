#include <fcntl.h> 
#include <fstream>
#include <linux/fb.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <sys/ioctl.h>
#include <opencv2/core/core_c.h>
#include <iostream>

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

int print_frame(cv::Mat& frame, std::ofstream& ofs, struct framebuffer_info& fb_info) {

    cv::Size2f frame_size = frame.size();
    cv::Mat frame_output;

	cv::cvtColor(frame, frame_output, cv::COLOR_BGR2BGR565);

    int fb_width = fb_info.xres_virtual;
    int fb_depth = fb_info.bits_per_pixel;
    int pixel_bytes = fb_depth / 8;

    for (int i = 0; i < frame_size.height; ++i) {
        // move ofs to ith row of framebuffer
        ofs.seekp(i * pixel_bytes * fb_width);
        // writing row by row
        // reinterpret : uchar* to char*
        ofs.write(reinterpret_cast<char*>(frame_output.ptr(i)), pixel_bytes * frame_size.width);
    }    

    return 0;
}

int main(int argc, char **argv) {

    // variable to store the frame get from video stream
    cv::Mat frame;

    // open video stream device
    // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a5d5f5dacb77bbebdcbfb341e3d4355c1
    // camera id ??
    cv::VideoCapture camera (2);

    // get info of the framebuffer
    framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");

    // open the framebuffer device
    // http://www.cplusplus.com/reference/fstream/ofstream/ofstream/
    std::ofstream ofs("/dev/fb0");

    // check if video stream device is opened success or not
    // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a9d2ca36789e7fcfe7a7be3b328038585
    if (!camera.isOpened()) {
        std::cerr << "Could not open video device." << std::endl;
        return 1;
    }

    // set frame property
    int frame_rate = 15;
    
    // scale 10 : 6
    int frame_width = 900;
    int frame_height = 540;

    // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a8c6d8c2d37505b5ca61ffd4bb54e9a7c
    // https://docs.opencv.org/3.4.7/d4/d15/group__videoio__flags__base.html#gaeb8dd9c89c10a5c63c139bf7c4f5704d
    camera.set(CV_CAP_PROP_FRAME_WIDTH, frame_width);
    camera.set(CV_CAP_PROP_FRAME_HEIGHT, frame_height);
    camera.set(CV_CAP_PROP_FPS, frame_rate);


    while (1) {

        // get video frame from stream
        // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a473055e77dd7faa4d26d686226b292c1
        // camera.read
        // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a199844fb74226a28b3ce3a39d1ff6765
        // read next video
        camera >> frame;

        print_frame(frame, ofs, fb_info);

    }

    camera.release ();

    return 0;
}