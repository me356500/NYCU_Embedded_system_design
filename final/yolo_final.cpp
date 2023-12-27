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
int start_frame;

double frame_width, frame_height, frame_rate;

std::ofstream ofs("/dev/fb0");
framebuffer_info fb_info;

vector<string> class_list;
/*-------------------------------------------------*/

void print_frame(Mat &frame) {

    cvtColor(frame, frame, COLOR_BGR2BGR565);

    for (int i = start_frame; i < frame_height + start_frame; ++i) {
        ofs.seekp(i * 2 * fb_info.xres_virtual);
        ofs.write(reinterpret_cast<char *>(frame.ptr(i - start_frame)), 2 * frame_width);
    }

}

const vector<Scalar> colors = {Scalar(255, 255, 0), Scalar(0, 255, 0), 
                                Scalar(0, 255, 255), Scalar(255, 0, 0)};

float INPUT_WIDTH = 640.0;
float INPUT_HEIGHT = 640.0;
float SCORE_THRESHOLD = 0.2;
float NMS_THRESHOLD = 0.4;
float CONFIDENCE_THRESHOLD = 0.4;

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

// Draw the predicted bounding box
void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
    //Draw a rectangle displaying the bounding box
    rectangle(frame, Point(left, top), Point(right, bottom), Scalar(255, 178, 50), 3);
    
    //Get the label for the class name and its confidence
    string label = format("%.2f", conf);
    if (!class_list.empty())
    {
        CV_Assert(classId < (int)class_list.size());
        label = class_list[classId] + ":" + label;
    }
    
    //Display the label at the top of the bounding box
    int baseLine;
    Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
    top = max(top, labelSize.height);
    rectangle(frame, Point(left, top - round(1.5*labelSize.height)), Point(left + round(1.5*labelSize.width), top + baseLine), Scalar(255, 255, 255), FILLED);
    putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,0),1);
}

void postprocess(Mat& frame, const vector<Mat>& outs)
{
    vector<int> classIds;
    vector<float> confidences;
    vector<Rect> boxes;
    
    for (size_t i = 0; i < outs.size(); ++i)
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
    for (size_t i = 0; i < indices.size(); ++i)
    {
        int idx = indices[i];
        Rect box = boxes[idx];
        drawPred(classIds[idx], confidences[idx], box.x, box.y,
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
    VideoCapture camera(2);

    fb_info = get_framebuffer_info("/dev/fb0");

    if (!camera.isOpened()) {
        std::cerr << "Could not open camera.\n";
        return 1;
    }

    /*----------------------------------------*/
    frame_width = atof(argv[1]);
    frame_height = atof(argv[2]);
    frame_rate = atof(argv[3]);

    start_frame = 0;
    camera.set(CV_CAP_PROP_FRAME_WIDTH, frame_width);
    camera.set(CV_CAP_PROP_FRAME_HEIGHT, frame_height);
    camera.set(CV_CAP_PROP_FPS, frame_rate);

    // opencv related

    // class list 
    ifstream ifs("./config/classes.txt");
    string line;
    while (getline(ifs, line)) {
        class_list.emplace_back(line);
    }


    // load net
#ifdef ONNX
    cv::dnn::Net net = readNet(argv[4]);
    net.setPreferableBackend(DNN_BACKEND_OPENCV);
    net.setPreferableTarget(DNN_TARGET_CPU);

#else
    cv::dnn::Net net = cv::dnn::readNetFromDarknet(argv[4], argv[5]);
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    int detect_size = atoi(argv[6]);

    SCORE_THRESHOLD = atof(argv[7]);
    NMS_THRESHOLD = atof(argv[8]);
    CONFIDENCE_THRESHOLD = atof(argv[9]);

    //auto model = cv::dnn::DetectionModel(net);
    //model.setInputParams(1./255, cv::Size(416, 416), cv::Scalar(), true);

#endif
    //auto start = std::chrono::high_resolution_clock::now();


    while (true) {

        camera.read(frame);

 

#ifdef ONNX
        vector<Detection> output;
        detect(frame, net, output, class_list);

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


#else

        blobFromImage(frame, blob, 1 / 255.0, Size(detect_size, detect_size), Scalar(0, 0, 0), true, false);
        net.setInput(blob);

        vector<Mat> outs;
        net.forward(outs, getOutsname(net));
        
        postprocess(frame, outs);


#endif
       

        
        print_frame(frame);

       
    }

    camera.release();
    return 0;
}