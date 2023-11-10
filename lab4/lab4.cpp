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


mutex mutex_mv;
bool flag_end = 0, mvleft = 0, mvright = 0;;

void listen_keyboard_terminal() {

    //cout << "Press 'c' to screenshot\nPress 'Esc' to end the program\n";

    // https://man7.org/linux/man-pages/man3/termios.3.html
    // termios noncanonical mode
    struct termios old_tio, new_tio;

    int echoeflag = ECHOE;

    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;

    int mossdisable = ICANON | ECHO;
    // disable canonical
    new_tio.c_lflag &= (~ICANON);

    tcflush(STDIN_FILENO, TCIFLUSH);
    int testflag = ICANON;
    int echoflag = ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    while (1) {

        int key = ' ';
        if (read(STDIN_FILENO, &key, 1) == 1) {
            //cout << key << '\n';
        }
        // 480
        
        if (key == 'j') {
            mutex_mv.lock();
            mvleft = 1;
            mutex_mv.unlock();
        }
        if (key == 'l') {
            mutex_mv.lock();
            mvright = 1;
            mutex_mv.unlock();
        }
        else if (key == 27) {
            flag_end = 1;
            break;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

}

framebuffer_info fb_info;
cv::Mat image;
cv::Size2f image_size;


int main(int argc, char **argv) {

    // get info of the framebuffer
    fb_info = get_framebuffer_info("/dev/fb0");

    std::ofstream ofs("/dev/fb0");


    // bonus
    thread t_listen(listen_keyboard_terminal);

    // 3840 x 1080
    image = cv::imread("picture.png", cv::IMREAD_UNCHANGED);
    cv::cvtColor(image, image, cv::COLOR_BGRA2BGR565);
    

    vector<cv::Mat> scrollboard(3, image);

    cv::Mat frame;
    
    cv::hconcat(scrollboard, frame);

    // start from middle image
    int shift = 0;

    /*
        |        |        |        |
        |  IMG1  |  IMG2  |  IMG3  |
        |        |        |        |
        0      3839      7679     11519
    */

    // actually concat 2 img is enough XD, mod 3840

    while (1) {


        if (mvleft) {
            mutex_mv.lock();

            shift = (shift - 60) % 3840;

            mvleft = 0;
            mutex_mv.unlock();
        }

        if (mvright) {
            mutex_mv.lock();
            
            shift = (shift + 60) % 3840;

            mvright = 0;
            mutex_mv.unlock();
        }

        if (flag_end) {
            t_listen.join();
            break;
        }

        for (int i = 0; i < 1080; ++i) {
           
            // ith row of HDMI frame buffer 
            ofs.seekp(i * 2 * fb_info.xres_virtual);

            // shift the frame pointer
            // row, column
            ofs.write(reinterpret_cast<char*>(frame.ptr(i, shift)), 2 * 1920);
        }    

    }


    return 0;
}