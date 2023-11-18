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

int main(int argc, char **argv) {

    cv::Mat frame;

    cv::VideoCapture camera (0);

    if (!camera.isOpened()) {
        std::cerr << "Could not open camera.\n";
        return 1;
    }

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

        imshow("face detection", frame); 

        if (waitKey(1) == 27)
            break;

    }

    camera.release();

    return 0;
}