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
#include <opencv2/face.hpp>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <string>
#include <iostream>
#include <termios.h>

using namespace std;
using namespace cv;
using namespace cv::face;



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


/*-----------------------------------------*/
// center video
int start;

double frame_width, frame_height, frame_rate;

std::ofstream ofs("/dev/fb0");
framebuffer_info fb_info;


/*-------------------------------------------------*/

vector<Rect> faces, eyes, noses;
Mat faceROI;


CascadeClassifier face_cascade, eye_cascade, nose_cascade;

double scaleFactor = 1.1;
int minNeighbors = 10;
int flags = (0 | CASCADE_SCALE_IMAGE);

int nose_flags = (0 | CASCADE_FIND_BIGGEST_OBJECT);

Size faceSize(35, 35);
Size eyeSize(5, 5);
Size noseSize(5, 5);

// id, -->  <precision, name>
unordered_map<int, pair<int, string>> student;

// train model
string train_model_path = "./trained_faces.xml";


/*----------------------------------------------------*/

void print_frame(Mat &frame) {

    cvtColor(frame, frame, COLOR_BGR2BGR565);

    for (int i = start; i < frame_height + start; ++i) {
        ofs.seekp(i * 2 * fb_info.xres_virtual);
        ofs.write(reinterpret_cast<char *>(frame.ptr(i - start)), 2 * frame_width);
    }

}

void detect_frame(Mat &frame, Mat &cvt_frame, int i) {


    int h = faces[i].y + faces[i].height;
    int w = faces[i].x + faces[i].width;

    // mark face
    rectangle(cvt_frame, Point(faces[i].x, faces[i].y), 
        Point(w, h), Scalar(0, 255, 0), 2, 8, 0);

#ifdef EYES

    faceROI = frame(faces[i]);
    
    eye_cascade.detectMultiScale(faceROI, eyes, scaleFactor, minNeighbors, flags, eyeSize);
    

    // prevent detecting noses
    sort(eyes.begin(), eyes.end(),
        [](const Rect &l, const Rect &r) {
            return l.y < r.y;
        });
    
    int eyes_y = -1;

    if (eyes.size())
        eyes_y = eyes[0].y;

    // mark eyes
    for (int j = 0; j < min((int)eyes.size(), 2); ++j) {
        
        Point center(faces[i].x + eyes[j].x + eyes[j].width * 0.5, 
                faces[i].y + eyes[j].y + eyes[j].height * 0.5);
        
        int r = cvRound((eyes[j].width + eyes[j].height) * 0.25);
        
        circle(cvt_frame, center, r, Scalar(255, 0, 0), 4, 8, 0);
    }

#endif

#ifdef NOSES
    nose_cascade.detectMultiScale(faceROI, noses, scaleFactor, minNeighbors, nose_flags, noseSize);

    // mark noses
    for (int j = 0; j < noses.size(); ++j) {
        
        if (~eyes_y && eyes_y >= noses[j].y)
            continue;

        Point center(faces[i].x + noses[j].x + noses[j].width * 0.5, 
                faces[i].y + noses[j].y + noses[j].height * 0.5);
        
        int r = cvRound((noses[j].width + noses[j].height) * 0.25);
        
        circle(cvt_frame, center, r, Scalar(0, 0, 255), 4, 8, 0);
    }

#endif

}

void detect(Mat &frame, Mat &cvt_frame) {

    cvt_frame = frame;

    cvtColor(frame, frame, COLOR_BGR2GRAY);
    face_cascade.detectMultiScale(frame, faces, scaleFactor, minNeighbors, flags, faceSize);

    for (int i = 0; i < faces.size(); ++i) {

        detect_frame(frame, cvt_frame, i);
    }

}

void train(VideoCapture &camera, Mat &frame) {

    Ptr<FaceRecognizer> train_model = LBPHFaceRecognizer::create();
    Mat cvt_frame;

    string name;
    int trained_id, training_frame, persons, student_id;
    double precision;

    vector<Mat> images;
    vector<int> labels;

    cout << "Input training persons : ";
    cin >> persons;

    while (persons--) {

        cout << "Enter name : ";
        cin >> name;

        cout << "Enter student id : ";
        cin >> student_id;

        cout << "Enter number of training frame : ";
        cin >> training_frame;

        for (int i = 0; i < training_frame; ++i) {

            camera.read(frame);

            cvt_frame = frame;

            cvtColor(frame, frame, COLOR_BGR2GRAY);

            face_cascade.detectMultiScale(frame, faces, scaleFactor, minNeighbors, nose_flags, faceSize);
            
            if (faces.size()) {

                // process
                detect_frame(frame, cvt_frame, 0);

                // training data
                images.emplace_back(faceROI);
                labels.emplace_back(student_id);

                putText(cvt_frame, "training", Point(faces[0].x, faces[0].y),
                    FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 255, 0), 2);

            }
            
            print_frame(cvt_frame);

        }

        train_model->update(images, labels);
        //train_model->train(images, labels);
        train_model->save(train_model_path);

        faceROI = frame(faces[0]);
        train_model->predict(faceROI, student_id, precision);
        
        student[student_id] = make_pair(precision, name);

    }

}

void recognize(VideoCapture &camera, Mat &frame) {

    Ptr<LBPHFaceRecognizer> train_model = Algorithm::load<LBPHFaceRecognizer>(train_model_path);

    Mat cvt_frame, frameROI;
    double precision;
    int label, range;
    string message;

    //cout << "Enter range : ";
    //cin >> range;

    while (1) {

        camera.read(frame);

        // output
        cvt_frame = frame;

        // compare
        cvtColor(frame, frame, COLOR_BGR2GRAY);
        face_cascade.detectMultiScale(frame, faces, scaleFactor, minNeighbors, flags, faceSize);
        
        for (int i = 0; i < faces.size(); ++i) {

            detect_frame(frame, cvt_frame, i);

            label = train_model->predict(faceROI);
            train_model->predict(faceROI, label, precision);
            
            message = student[label].second + " : " + to_string(precision - student[label].first);

            putText(cvt_frame, message, Point(faces[i].x, faces[i].y),
                FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 255, 0), 2);
            
        }
        
        print_frame(cvt_frame);

    }
}

int main(int argc, char **argv) {


    cv::Mat frame, cvt_frame;

    cv::VideoCapture camera (2);

    fb_info = get_framebuffer_info("/dev/fb0");
    
    if (!camera.isOpened()) {
        std::cerr << "Could not open camera.\n";
        return 1;
    }

    // set frame property
    frame_width = atof(argv[1]);
    frame_height = atof(argv[2]);
    frame_rate = 30.0;

    start = 0;
    camera.set(CV_CAP_PROP_FRAME_WIDTH, frame_width);
    camera.set(CV_CAP_PROP_FRAME_HEIGHT, frame_height);
    camera.set(CV_CAP_PROP_FPS, frame_rate);


    // load detector
    face_cascade.load("./haarcascades/haarcascade_frontalface_alt.xml");
    eye_cascade.load("./haarcascades/haarcascade_eye_tree_eyeglasses.xml");
    nose_cascade.load("./haarcascades/haarcascade_mcs_nose.xml");


    int control = atoi(argv[3]);

    minNeighbors = atoi(argv[4]);

    scaleFactor = atof(argv[5]);


    if (control == 1) {

        while (1) {

            camera.read(frame);

            detect(frame, cvt_frame);

            print_frame(cvt_frame);

        }

    }
    else {

        train(camera, frame);

        recognize(camera, frame);

    }

    camera.release();

    return 0;
}