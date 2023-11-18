#include <fcntl.h> 
#include <fstream>
#include <linux/fb.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <sys/ioctl.h>
#include <unistd.h>
#include <opencv2/core/core_c.h>
#include <opencv2/objdetect.hpp>
#include <mutex>
#include <thread>
#include <string>
#include <iostream>
#include <termios.h>

using namespace std;
using namespace cv;

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

int main(int argc, char **argv) {


    cv::Mat frame;

    cv::VideoCapture camera (2);

    // get info of the framebuffer
    framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
    std::ofstream ofs("/dev/fb0");

    // check if video stream device is opened success or not
    // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a9d2ca36789e7fcfe7a7be3b328038585
    if (!camera.isOpened()) {
        std::cerr << "Could not open camera.\n";
        return 1;
    }

    // set frame property
    
    
    // scale 10 : 6
    double frame_width = atof(argv[1]);
    double frame_height = atof(argv[2]);
    double frame_rate = atof(argv[3]);

    int start = atoi(argv[4]);
    // https://docs.opencv.org/3.4.7/d8/dfe/classcv_1_1VideoCapture.html#a8c6d8c2d37505b5ca61ffd4bb54e9a7c
    // https://docs.opencv.org/3.4.7/d4/d15/group__videoio__flags__base.html#gaeb8dd9c89c10a5c63c139bf7c4f5704d
    camera.set(CV_CAP_PROP_FRAME_WIDTH, frame_width);
    camera.set(CV_CAP_PROP_FRAME_HEIGHT, frame_height);
    camera.set(CV_CAP_PROP_FPS, frame_rate);

    // detector
    CascadeClassifier face_detector;
    face_detector.load("haarcascade_frontalface_alt.xml");

    // detected faces rectangle
    vector<Rect> faces;

    while (1) {


        camera.read(frame);

        face_detector.detectMultiScale(frame, faces, 1.1, 4, CASCADE_SCALE_IMAGE, Size(30, 30));

        for (int i = 0; i < faces.size(); ++i) {
            int h = faces[i].y + faces[i].height;
            int w = faces[i].x + faces[i].width;
            rectangle(frame, Point(faces[i].x, faces[i].y), Point(w, h), Scalar(0, 255, 0), 2, 8, 0); 
        }

        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGR565);
 
        
        for (int i = start; i < frame_height + start; ++i) {
            ofs.seekp(i * 2 * fb_info.xres_virtual + 640);
            ofs.write(reinterpret_cast<char*>(frame.ptr(i - start)), 2 * frame_width);
        } 

        if (waitKey(10) == 27)
            break;

    }

    camera.release();

    return 0;
}