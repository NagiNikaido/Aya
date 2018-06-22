#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <string>

typedef cv::Point3_<uint8_t> Pixel;
typedef cv::Point_<uint16_t> Pos;

const double _To_Be_Preserved = 1e6;
const double _To_Be_Removed = -1;

void Main(const std::string &pic_name, const std::string &mask_name, const std::string &output_name, const std::string &process_name, float scale);