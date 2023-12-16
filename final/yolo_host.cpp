#include <fstream>

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;
using namespace cv::dnn;


const vector<Scalar> colors = {Scalar(255, 255, 0), Scalar(0, 255, 0), 
                                Scalar(0, 255, 255), Scalar(255, 0, 0)};

const float INPUT_WIDTH = 640.0;
const float INPUT_HEIGHT = 640.0;
const float SCORE_THRESHOLD = 0.2;
const float NMS_THRESHOLD = 0.4;
const float CONFIDENCE_THRESHOLD = 0.4;

struct Detection
{
    int class_id;
    float confidence;
    cv::Rect box;
};

Mat format_yolov5(const Mat &source) {
    int col = source.cols;
    int row = source.rows;
    int _max = MAX(col, row);
    Mat result = cv::Mat::zeros(_max, _max, CV_8UC3);
    source.copyTo(result(Rect(0, 0, col, row)));
    return result;
}

void detect(Mat &image, Net &net, vector<Detection> &output, const vector<string> &className) {
    
    Mat blob;

    auto input_image = format_yolov5(image);
    
    blobFromImage(input_image, blob, 1./255., Size(INPUT_WIDTH, INPUT_HEIGHT), Scalar(), true, false);
    net.setInput(blob);
    vector<Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    float x_factor = input_image.cols / INPUT_WIDTH;
    float y_factor = input_image.rows / INPUT_HEIGHT;
    
    float *data = (float *)outputs[0].data;

    const int dimensions = 85;
    const int rows = 25200;
    
    vector<int> class_ids;
    vector<float> confidences;
    vector<Rect> boxes;

    for (int i = 0; i < rows; ++i) {

        float confidence = data[4];
        if (confidence >= CONFIDENCE_THRESHOLD) {

            float * classes_scores = data + 5;
            Mat scores(1, className.size(), CV_32FC1, classes_scores);
            Point class_id;
            double max_class_score;
            minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
            if (max_class_score > SCORE_THRESHOLD) {

                confidences.emplace_back(confidence);

                class_ids.emplace_back(class_id.x);

                float x = data[0];
                float y = data[1];
                float w = data[2];
                float h = data[3];
                int left = int((x - 0.5 * w) * x_factor);
                int top = int((y - 0.5 * h) * y_factor);
                int width = int(w * x_factor);
                int height = int(h * y_factor);
                boxes.emplace_back(Rect(left, top, width, height));
            }

        }

        data += 85;

    }

    vector<int> nms_result;
    cv::dnn::NMSBoxes(boxes, confidences, SCORE_THRESHOLD, NMS_THRESHOLD, nms_result);

    for (int i = 0; i < nms_result.size(); i++) {
        int idx = nms_result[i];
        Detection result;
        result.class_id = class_ids[idx];
        result.confidence = confidences[idx];
        result.box = boxes[idx];
        output.push_back(result);
    }
}

int main(int argc, char **argv) {

    Mat frame;
    VideoCapture camera(0);

    if (!camera.isOpened()) {
        std::cerr << "Could not open camera.\n";
        return 1;
    }


    vector<string> class_list;

    // class list 
    ifstream ifs("config/classes.txt");
    string line;
    while (getline(ifs, line)) {
        class_list.emplace_back(line);
    }


    // load net
    cv::dnn::Net net = readNet("config/yolov5s.onnx");
    net.setPreferableBackend(DNN_BACKEND_OPENCV);
    net.setPreferableTarget(DNN_TARGET_CPU);


    auto start = std::chrono::high_resolution_clock::now();
    int frame_count = 0,  total_frames = 0;
    float fps = -1;

    while (true) {

        camera.read(frame);


        vector<Detection> output;
        detect(frame, net, output, class_list);

        frame_count++;
        total_frames++;

        int detections = output.size();

        for (int i = 0; i < detections; ++i)
        {

            auto detection = output[i];
            auto box = detection.box;
            auto classId = detection.class_id;
            const auto color = colors[classId % colors.size()];
            rectangle(frame, box, color, 3);

            rectangle(frame, Point(box.x, box.y - 20), 
                            Point(box.x + box.width, box.y), color, cv::FILLED);

            putText(frame, class_list[classId].c_str(), 
                            Point(box.x, box.y - 5), 
                            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
        }

        if (fps > 0)
        {

            ostringstream fps_label;
            fps_label << fixed << setprecision(2);
            fps_label << "FPS: " << fps;
            string fps_label_str = fps_label.str();

            putText(frame, fps_label_str.c_str(), 
                            Point(10, 25), 
                            FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
        }

        imshow("output", frame);

        if (waitKey(1) == 27) {
            break;
        }
    }

    camera.release();
    return 0;
}