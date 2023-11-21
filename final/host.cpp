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
#include <string>
#include <iostream>
#include <unordered_map>
#include <termios.h>

using namespace std;
using namespace cv;
using namespace cv::face;



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

// student_id, -->  <precision, name>
unordered_map<int, pair<int, string>> student;

// train model
string train_model_path = "./trained_faces.xml";


/*----------------------------------------------------*/

void detect_frame(Mat &frame, Mat &cvt_frame, int i) {


    int h = faces[i].y + faces[i].height;
    int w = faces[i].x + faces[i].width;

    // mark face
    rectangle(cvt_frame, Point(faces[i].x, faces[i].y), 
        Point(w, h), Scalar(0, 255, 0), 2, 8, 0);

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
    int training_frame, persons, student_id;
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

        namedWindow("training", WINDOW_KEEPRATIO);
        resizeWindow("training", 800, 800); 

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
            

            imshow("training", cvt_frame);
            

            if (waitKey(10) == 27)
                break;

        }

       

        train_model->update(images, labels);
        //train_model->train(images, labels);
        train_model->save(train_model_path);

        faceROI = frame(faces[0]);
        train_model->predict(faceROI, student_id, precision);
        
        student[student_id] = make_pair(precision, name);

    }

    destroyWindow("training");

}

void recognize(VideoCapture &camera, Mat &frame) {

    Ptr<LBPHFaceRecognizer> train_model = Algorithm::load<LBPHFaceRecognizer>(train_model_path);

    Mat cvt_frame, frameROI;
    double precision;
    int student_id, range;
    string message;

    //cout << "Enter range : ";
    //cin >> range;

    namedWindow("face detection", WINDOW_KEEPRATIO);
    resizeWindow("face detection", 800, 800); 

    while (1) {

        camera.read(frame);

        // output
        cvt_frame = frame;
        // compare
        cvtColor(frame, frame, COLOR_BGR2GRAY);
        face_cascade.detectMultiScale(frame, faces, scaleFactor, minNeighbors, flags, faceSize);
        
        for (int i = 0; i < faces.size(); ++i) {

            detect_frame(frame, cvt_frame, i);

            student_id = train_model->predict(faceROI);
            train_model->predict(faceROI, student_id, precision);
            
            if (!student.count(student_id))
                message = "not matched";
            else 
                message = student[student_id].second + " : " + to_string(precision - student[student_id].first);

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
    face_cascade.load("./haarcascades/haarcascade_frontalface_alt.xml");
    eye_cascade.load("./haarcascades/haarcascade_eye_tree_eyeglasses.xml");
    nose_cascade.load("./haarcascades/haarcascade_mcs_nose.xml");


    namedWindow("test", WINDOW_KEEPRATIO);
    resizeWindow("test", 800, 800);

    while (1) {

        camera.read(frame);
        
        detect(frame, cvt_frame);

        imshow("test", cvt_frame);

        if (waitKey(10) == 27)
            break;

    }

    destroyWindow("test");

    
    train(camera, frame);
    recognize(camera, frame);


    camera.release();

    return 0;
}