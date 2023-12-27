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
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <opencv2/dnn.hpp>
#include <mutex>
#include <thread>
#include <string>
#include <iostream>
#include <unordered_map>
#include <map>
#include <termios.h>

using namespace std;
using namespace cv;
using namespace cv::face;
using namespace cv::dnn;



/*------------Global var----------------------------*/

vector<Rect> faces;
Mat faceROI;


CascadeClassifier face_cascade;

/*------------Cascade flags-------------------------*/

double scaleFactor = 1.1;
int minNeighbors = 2;
int flags = (0 | CASCADE_SCALE_IMAGE);

Size faceSize(100, 100);


/*------------Emotion detect------------------------*/

Net network;
map<int , string> classid_to_string;

/*--------------------------------------------------*/

void detect_frame(Mat &frame, Mat &cvt_frame, int i) {

    int h = faces[i].y + faces[i].height;
    int w = faces[i].x + faces[i].width;

    // mark face
    rectangle(cvt_frame, Point(faces[i].x, faces[i].y), 
        Point(w, h), Scalar(0, 255, 0), 2, 8, 0);

    
    faceROI = frame(faces[i]);

}


void recognize(VideoCapture &camera, Mat &frame) {

    Mat cvt_frame;
    string message;

    namedWindow("face detection", WINDOW_KEEPRATIO);
    resizeWindow("face detection", 800, 800); 

    while (1) {

        camera.read(frame);

        // output
        cvt_frame = frame;

        // compare
        cvtColor(frame, frame, COLOR_BGR2GRAY);

        // enhance contrast ??
        equalizeHist(frame, frame);

        face_cascade.detectMultiScale(frame, faces, scaleFactor, minNeighbors, flags, faceSize);
        
        for (int i = 0; i < faces.size(); ++i) {

            detect_frame(frame, cvt_frame, i);
           
            /*------Preprocess ROI-------*/
            // model base on 48 x 48
            resize(faceROI, faceROI, Size(48, 48));
            faceROI.convertTo(faceROI, CV_32FC3, 1.f/255);

            /*------Predict--------------*/

            Mat blob = blobFromImage(faceROI);

            network.setInput(blob);

            Mat prob = network.forward();

            /*------Rank probabilities-------*/
            // copy paste
            Mat sorted_probabilities;
            cv::Mat sorted_ids;
            cv::sort(prob.reshape(1, 1), sorted_probabilities, cv::SORT_DESCENDING);
            cv::sortIdx(prob.reshape(1, 1), sorted_ids, cv::SORT_DESCENDING);

            // Get top probability and top class id
            float top_probability = sorted_probabilities.at<float>(0);
            int top_class_id = sorted_ids.at<int>(0);

            // Prediction result string to print
            string class_name = classid_to_string.at(top_class_id);
            message = class_name + ": " + std::to_string(top_probability * 100) + "%";

            putText(cvt_frame, message, Point(faces[i].x, faces[i].y),
                FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 255, 0), 2);

        }
        
        imshow("face detection", cvt_frame); 

        if (waitKey(1) == 27)
            break;

    }
}

int main(int argc, char **argv) {

    Mat frame, cvt_frame;

    cv::VideoCapture camera (0);

    if (!camera.isOpened()) {
        std::cerr << "Could not open camera.\n";
        return 1;
    }

    // detector
    face_cascade.load("./haarcascades/haarcascade_frontalface_alt2.xml");


    classid_to_string[0] = "Angry";
    classid_to_string[1] = "Disgust";
    classid_to_string[2] = "Fear";
    classid_to_string[3] = "Happy";
    classid_to_string[4] = "Sad";
    classid_to_string[5] = "Surprise";
    classid_to_string[6] = "Neutral";

    network = readNet("./tensorflow_model.pb");

    if (network.empty()) {
        cerr << "Could not load the network." << '\n';
        return -1;
    }

    recognize(camera, frame);

    camera.release();

    return 0;
}