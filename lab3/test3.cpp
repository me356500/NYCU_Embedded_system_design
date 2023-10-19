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

int screenshot_cnt = 0;

void screenshot(cv::Mat& frame) {

    string filename = "/run/media/mmcblk1p1/screenshot/" + to_string(screenshot_cnt++) + ".bmp";

    // https://docs.opencv.org/3.4/d4/da8/group__imgcodecs.html#gabbc7ef1aa2edfaa87772f1202d67e0ce
    if (cv::imwrite(filename, frame))
        cout << "Screenshot : " << filename << '\n';
    else 
        cout << "[Error] Screenshot failed : " << filename << '\n';

}

mutex mutex_screenshot, mutex_end;
bool flag_screenshot = 0, flag_end = 0;

void listen_keyboard_opencv() {

    cout << "Press 'c' to screenshot\nPress 'Esc' to end the program\n";

    // https://docs.opencv.org/4.x/d7/dfc/group__highgui.html
    cv::namedWindow("Listen keyboard Window",cv::WINDOW_AUTOSIZE);

    while (1) {
        // polling wait, get ascii code
        int key = cv::waitKey();
        cout << key << '\n';

        if (key == 'c') {
            mutex_screenshot.lock();
            flag_screenshot = 1;
            mutex_screenshot.unlock();
        }
        else if (key == 27) {
            mutex_end.lock();
            flag_end = 1;
            mutex_end.unlock();
            break;
        }
    }

    cv::destroyAllWindows();
}

void listen_keyboard_terminal() {

    cout << "Press 'c' to screenshot\nPress 'Esc' to end the program\n";

    // https://man7.org/linux/man-pages/man3/termios.3.html
    // termios noncanonical mode
    struct termios old_tio, new_tio;

    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;

    // disable canonical
    new_tio.c_lflag &= (~ICANON);

    tcflush(STDIN_FILENO, TCIFLUSH);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    while (1) {

        int key = ' ';
        if (read(STDIN_FILENO, &key, 1) == 1) {
            cout << key << '\n';
        }
        
        if (key == 'c') {
            mutex_screenshot.lock();
            flag_screenshot = 1;
            mutex_screenshot.unlock();
        }
        else if (key == 27) {
            mutex_end.lock();
            flag_end = 1;
            mutex_end.unlock();
            break;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

}

cv::Mat print_image(const string &filename, std::ofstream& ofs, struct framebuffer_info& fb_info) {

    cv::Mat image, image_convert;
    cv::Size2f image_size;

    int fb_width = fb_info.xres_virtual;
    int pixel_bytes = fb_info.bits_per_pixel / 8;
    int cvtcode;
    
    if (filename.substr(filename.size() - 3) == "bmp") {
        image = cv::imread(filename, cv::IMREAD_COLOR);
        cvtcode = cv::COLOR_BGR2BGR565;
    }
    else {
        image = cv::imread(filename, cv::IMREAD_UNCHANGED);
        cvtcode = cv::COLOR_BGRA2BGR565;
    }

    image_size = image.size();
    
    cv::cvtColor(image, image_convert, cvtcode);

    for (int i = 0; i < image_size.height; ++i) {
        ofs.seekp(i * pixel_bytes * fb_width);
        ofs.write(reinterpret_cast<char*>(image_convert.ptr(i)), pixel_bytes * image_size.width);
    }    

    return image_convert;
}


int main(int argc, char **argv) {

    /*  Test some features
        
        1. HDMI output
        2. thread screenshot
        3. getch() screenshot
        4. video recording
        5. video scale and frame scale
    
    */
    
    cv::Mat frame;
    cv::Size2f image_size;

    // get info of the framebuffer
    framebuffer_info fb_info = get_framebuffer_info("/dev/fb0");
    std::ofstream ofs("/dev/fb0");

    // preprocess image
    cv::Mat frame_png = print_image("advance.png", ofs, fb_info);
    cv::Mat frame_bmp = print_image("NYCU_logo.bmp", ofs, fb_info);
    cv::Mat frame_pepe = print_image("pepe.png", ofs, fb_info);

    // set frame property
    double frame_rate = 10.0;
    
    // scale 10 : 6
    double frame_width = 480.0;
    double frame_height = 480.0;


    // thread test
    thread t_listen;

    /*
        0       : opencv listen
        1       : terminal listen
        others  : getch() 
    */

    if (argv[1][0] == '0') {
        t_listen = thread(listen_keyboard_opencv);
    }
    else if (argv[1][0] == '1') {
        t_listen = thread(listen_keyboard_terminal);
    }


    cv::Size video_size = cv::Size(frame_width, frame_height);

    // motion jpeg
    // if MJPG doesn't work, try (*'XVID)
    int fourcc_code = cv::VideoWriter::fourcc('M', 'J', 'P', 'G'); 

    // https://docs.opencv.org/3.4/dd/d9e/classcv_1_1VideoWriter.html#af52d129e2430c0287654e7d24e3bbcdc
    // https://docs.opencv.org/3.4/df/d94/samples_2cpp_2videowriter_basic_8cpp-example.html#a5
    // OpenCV supported avi only
    cv::VideoWriter video("/run/media/mmcblk1p1/video/bonus.avi", fourcc_code, frame_rate, video_size, true);
    
    // input bmp and png alternatively
    int cnt = 0;
    unsigned char c = ' ';

    while (1) {

        ++cnt %= 3;

        switch (cnt % 3) {
            
            case 0:
                frame = frame_png;
                break;
            
            case 1:
                frame = frame_bmp;
                break;

            case 2:
                frame = frame_pepe;
                break;
            
            default:
                break;

        }

        // https://docs.opencv.org/3.4/dd/d9e/classcv_1_1VideoWriter.html#a3115b679d612a6a0b5864a0c88ed4b39
        video.write(frame);

        if (flag_screenshot) {
            screenshot(frame);
            mutex_screenshot.lock();
            flag_screenshot = 0;
            mutex_screenshot.unlock();
        }

        if (flag_end) {
            cout << "Program End\n";
            t_listen.join();
            break;
        }

#ifdef NO_THREAD

        c = getch();

        if (c == 'c') {
            screenshot(frame);
        }

        if (c == 27) {
            cout << "Program End\n";
            break;
        }

#endif

        // display frame
        image_size = frame.size();

        for (int i = 0; i < image_size.height; ++i) {
            ofs.seekp(i * 2 * fb_info.xres_virtual);
            ofs.write(reinterpret_cast<char*>(frame.ptr(i)), 2 * image_size.width);
        } 

    }

    video.release();

    return 0;
}