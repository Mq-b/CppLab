#include <opencv2/opencv.hpp>
#include <iostream>
#include <format>

int main() {
    cv::VideoCapture cap(2);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera." << std::endl;
        return -1;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 2560);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1440);

    //获取实际分辨率
    const double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    const double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    std::cout << "分辨率设置为：" << width << "x" << height << std::endl;
    // 图片数量
    int image_count = 1;
    while (true) {
        cv::Mat frame;
        cap >> frame;  // 从摄像头捕获一帧图像

        if (frame.empty()) {
            std::cerr << "无法获取图像!" << std::endl;
            break;
        }

        // 显示图像
        cv::imshow("2K Video", frame);

        // 按 'q' 键退出
        if (cv::waitKey(100) == 'q') {
            break;
        }
        if (cv::waitKey(100) == 's') {
            // 保存一张图片
            cv::imwrite(std::format("captured_image{}.png", image_count++), frame);
        }
    }

    cap.release();  // 释放摄像头资源
    cv::destroyAllWindows();  // 销毁所有窗口
}