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

vector<Rect> faces, eyes;
Mat faceROI;

CascadeClassifier face_cascade, eye_cascade, nose_cascade;

// id, -->  <precision, name>
unordered_map<int, pair<int, string>> student;

// train model
string train_model_path = "./trained_faces.xml";


void detect(Mat &frame) {

    face_cascade.detectMultiScale(frame, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(35, 35));

    for (int i = 0; i < faces.size(); ++i) {

        // faces
        int h = faces[i].y + faces[i].height;
        int w = faces[i].x + faces[i].width;

        rectangle(frame, Point(faces[i].x, faces[i].y), 
            Point(w, h), Scalar(0, 255, 0), 2, 8, 0); 
        
        // eyes
        faceROI = frame(faces[i]);
        eye_cascade.detectMultiScale(faceROI, eyes, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(5, 5));

        for (int j = 0; j < eyes.size(); ++j) {
            
            Point center(faces[i].x + eyes[j].x + eyes[j].width * 0.5, 
                    faces[i].y + eyes[j].y + eyes[j].height * 0.5);
            
            int r = cvRound((eyes[j].width + eyes[j].height) * 0.25);
            
            circle(frame, center, r, Scalar(255, 0, 0), 4, 8, 0);
        }
    }

    eyes.clear();
    faces.clear();

}

void train(VideoCapture &camera, Mat &frame) {

    Ptr<FaceRecognizer> train_model = LBPHFaceRecognizer::create();
    Mat faceROI;

    string name, student_id;
    int trained_id, training_frame, persons;
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

            cvtColor(frame, frame, COLOR_BGR2GRAY);
            face_cascade.detectMultiScale(frame, faces, 1.1, 3, 0 | CASCADE_SCALE_IMAGE, Size(35, 35));
            
            
            if (faces.size()) {
                cout << faces[i].x << ", " << faces[i].y << "\n";

                faceROI = frame(faces[i]);
                
                images.emplace_back(faceROI);
                labels.emplace_back(persons);

            }

        }

       

        train_model->update(images, labels);
        train_model->save(train_model_path);

        faceROI = frame(faces[0]);
        train_model->predict(faceROI, persons, precision);
        
        student[persons] = make_pair(precision, name);

        faces.clear();
    }


}

void recognize(VideoCapture &camera, Mat &frame) {

    Ptr<LBPHFaceRecognizer> train_model = Algorithm::load<LBPHFaceRecognizer>(train_model_path);

    Mat cvt_frame, frameROI;
    double precision;
    int label, range;
    string message;

    cout << "Enter range : ";
    cin >> range;

    while (1) {

        camera.read(frame);

        // output
        //cvtColor(frame, cvt_frame, COLOR_BGR2BGR565);
        cvt_frame = frame;
        // compare
        cvtColor(frame, frame, COLOR_BGR2GRAY);
        face_cascade.detectMultiScale(frame, faces, 1.1, 3, 0 | CASCADE_SCALE_IMAGE, Size(35, 35));
        


        for (int i = 0; i < faces.size(); ++i) {

            // faces
            int h = faces[i].y + faces[i].height;
            int w = faces[i].x + faces[i].width;

            rectangle(cvt_frame, Point(faces[i].x, faces[i].y), 
                Point(w, h), Scalar(0, 255, 0), 2, 8, 0);

            faceROI = frame(faces[i]);

            label = train_model->predict(faceROI);
            train_model->predict(faceROI, label, precision);
            
            message = "unknown";

            if (precision > student[label].first + range)
                message = student[label].second;

            putText(cvt_frame, message, Point(faces[i].x, faces[i].y),
                FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 255, 0), 2);
            


        }

        faces.clear();
         
        imshow("face detection", cvt_frame); 

        if (waitKey(1) == 27)
            break;

    }
}

int main(int argc, char **argv) {

    Mat frame;

    cv::VideoCapture camera (0);

    if (!camera.isOpened()) {
        std::cerr << "Could not open camera.\n";
        return 1;
    }

    // detector
    face_cascade.load("./haarcascades/haarcascade_frontalface_alt.xml");
    eye_cascade.load("./haarcascades/haarcascade_eye_tree_eyeglasses.xml");



    //while (1) {

        train(camera, frame);
        recognize(camera, frame);
    //}

    camera.release();

    return 0;
}