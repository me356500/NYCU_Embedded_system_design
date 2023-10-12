#include <fcntl.h> 
#include <fstream>
#include <linux/fb.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
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

int print_image(const string &filename, std::ofstream& ofs, struct framebuffer_info& fb_info) {

    cv::Mat image;
    cv::Size2f img_size;

    // imread(filename, flags) COLOR, GRAYSCALE,UNCHANGED
    image = cv::imread(filename, cv::IMREAD_COLOR);

    // opencv mat size
    image_size = image.size();

    int fb_width = fb_info.xres_virtual;
    int fb_depth = fb_info.bits_per_pixel;
    int pixel_bytes = fb_depth / 8;

    // cvtColor(src_img, dst_img, coversion code)
    // https://docs.opencv.org/3.4.7/d8/d01/group__imgproc__color__conversions.html#ga4e0972be5de079fed4e3a10e24ef5ef0
    cv::Mat image_convert;

    int cvtcode;
    
    if (filename.substr(filename.size() - 3) == "bmp") {

        cvtcode = cv::COLOR_BGR2BGR565;
    }
    else {
        
        cvtcode = cv::COLOR_BGRA2BGR565;
    }


    // BGR to BGR565 (16-bit image)
    // bmp : no compression using BGR
    cv::cvtColor(image, image_convert, cvtcode);

    // https://docs.opencv.org/3.4.7/d3/d63/classcv_1_1Mat.html#a13acd320291229615ef15f96ff1ff738

    for (int i = 0; i < image_size.height; ++i) {
        // move ofs to ith row of framebuffer
        ofs.seekp(i * pixel_bytes * fb_width);
        // writing row by row
        // reinterpret : uchar* to char*
        ofs.write(reinterpret_cast<char*>(image_convert.ptr(i)), pixel_bytes * image_size.width);
    }    

    return 0;
}


int main(int argc, char ** argv) {

    framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
    std::ofstream ofs("/dev/fb0");

    print_image(string(argv[1]), ofs, fb_info);
    
    return 0;
}