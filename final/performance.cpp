#include <fstream>

#include <opencv2/opencv.hpp>

#include <fcntl.h> 
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>


using namespace std;
using namespace cv;
using namespace cv::dnn;



/*-----------------------------------------*/
// center video



vector<string> class_list;
/*-------------------------------------------------*/



float SCORE_THRESHOLD = 0.2;
float NMS_THRESHOLD = 0.4;
float CONFIDENCE_THRESHOLD = 0.4;


// Draw the predicted bounding box
void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
    //Draw a rectangle displaying the bounding box
    rectangle(frame, Point(left, top), Point(right, bottom), Scalar(255, 178, 50), 3);
    
    //Get the label for the class name and its confidence
    string label = format("%.2f", conf);
    if (!class_list.empty())
    {
        //CV_Assert(classId < (int)class_list.size());
        label = class_list[classId] + ":" + label;
    }
    
    //Display the label at the top of the bounding box
    int baseLine;
    Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
    top = max(top, labelSize.height);
    rectangle(frame, Point(left, top - round(1.5 * labelSize.height)), Point(left + round(1.5*labelSize.width), top + baseLine), Scalar(255, 255, 255), FILLED);
    putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,0),1);
}

void postprocess(Mat& frame, const vector<Mat>& outs)
{
    vector<int> classIds;
    vector<float> confidences;
    vector<Rect> boxes;
    
    int out_len = outs.size();
    for (size_t i = 0; i < out_len; ++i)
    {
        // Scan through all the bounding boxes output from the network and keep only the
        // ones with high confidence scores. Assign the box's class label as the class
        // with the highest score for the box.
        float* data = (float*)outs[i].data;
        for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
        {
            Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
            Point classIdPoint;
            double confidence;
            // Get the value and location of the maximum score
            minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
            if (confidence > CONFIDENCE_THRESHOLD)
            {
                int centerX = (int)(data[0] * frame.cols);
                int centerY = (int)(data[1] * frame.rows);
                int width = (int)(data[2] * frame.cols);
                int height = (int)(data[3] * frame.rows);
                int left = centerX - width / 2;
                int top = centerY - height / 2;
                
                classIds.emplace_back(classIdPoint.x);
                confidences.emplace_back((float)confidence);
                boxes.emplace_back(Rect(left, top, width, height));
            }
        }
    }
        // Perform non maximum suppression to eliminate redundant overlapping boxes with
    // lower confidences
    vector<int> indices;
    NMSBoxes(boxes, confidences, CONFIDENCE_THRESHOLD, NMS_THRESHOLD, indices);

    int indices_len = indices.size();
    for (size_t i = 0; i < indices_len; ++i) {
        Rect box = boxes[indices[i]];
        drawPred(classIds[indices[i]], confidences[indices[i]], box.x, box.y,
                 box.x + box.width, box.y + box.height, frame);
    }
}

vector<String> getOutsname(const Net& net) {

    static vector<String> names;

    if (names.empty()) {
        
        vector<int> outLayer = net.getUnconnectedOutLayers();

        vector<String> layersname = net.getLayerNames();

        names.resize(outLayer.size());

        for (int i = 0; i < outLayer.size(); ++i)
            names[i] = layersname[outLayer[i] - 1];
    }

    return names;
}

int main(int argc, char **argv) {

    Mat frame, cvt_frame, blob;
    

    SCORE_THRESHOLD = atof(argv[1]); //0.2
    NMS_THRESHOLD = atof(argv[2]);  //0.4
    CONFIDENCE_THRESHOLD = atof(argv[3]); //0.4




    // class list 
    ifstream ifs("./config/classes.txt");
    string line;
    while (getline(ifs, line)) {
        class_list.emplace_back(line);
    }


    cv::dnn::Net net = cv::dnn::readNetFromDarknet(argv[4], argv[5]);
    

    int control = atoi(argv[6]);

    if (control == 0) {
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    }
    else if (control == 1) {
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    }
    else if (control == 2) {
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL);
    }
    else if (control == 3) {
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL);
    }
    

    frame = cv::imread("example001.png", cv::IMREAD_UNCHANGED);

    int blob_size = atoi(argv[7]);

    blobFromImage(frame, blob, 1 / 255.0, Size(blob_size, blob_size), Scalar(0, 0, 0), true, false);
    net.setInput(blob);

    vector<Mat> outs;
    net.forward(outs, getOutsname(net));
    
    postprocess(frame, outs);


    cv::imwrite("detect.png", frame);
 

       
    
    return 0;
}