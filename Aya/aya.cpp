#include "aya.h"
#include <string>
#include <ctime>
#include <vector>
#include <iostream>

using namespace std;

static cv::Mat pic, gray, mask, process, pos;
static cv::Mat gradient, ene, trace;
static vector<int> pathx, pathy;
static bool useMask ,downwards, useForbid;
static int deltax, deltay;
static float energx, energy;

template<typename T>
bool ckmin(T &a, T b) {
	return b < a ? a = b, true : false;
}

Pixel randColor() {
	return Pixel(rand() & 0xff, rand() & 0xff, rand() & 0xff);
}

void init() {
	//cout << "init!" << endl;
	static cv::Mat kernel_x = (cv::Mat_<float>(3, 3) << 0, 0, 0, 0, 1, -1, 0, 0, 0);
	static cv::Mat kernel_y = (cv::Mat_<float>(3, 3) << 0, 0, 0, 0, 1, 0, 0, -1, 0);
	gray = cv::Mat(pic.rows, pic.cols, CV_32F, cv::Scalar(0));
	cv::cvtColor(pic, gray, CV_BGR2GRAY);
	cv::Mat g_x(pic.rows, pic.cols, CV_32F, cv::Scalar(0));
	cv::Mat g_y(pic.rows, pic.cols, CV_32F, cv::Scalar(0));
	cv::filter2D(gray, g_x, g_x.depth(), kernel_x);
	cv::filter2D(gray, g_y, g_y.depth(), kernel_y);
	gradient = cv::Mat(pic.rows, pic.cols, CV_32F, cv::Scalar(0));
	cv::add(cv::abs(g_x), cv::abs(g_y), gradient);
	//cout << gray << endl;
	//cout << gradient << endl;
	if (useMask) {
		gradient.forEach<float>([&](float &g, const int p[])-> void {
			if (mask.at<Pixel>(p[0], p[1]) == Pixel(0, 255, 0)) {
				g = max(g * _To_Be_Preserved, _To_Be_Preserved);
			}
			if (mask.at<Pixel>(p[0], p[1]) == Pixel(0, 0, 255)) {
				g *= _To_Be_Removed;
			}
		});
	}
	ene = cv::Mat(pic.rows, pic.cols, CV_32F, cv::Scalar(0));
	trace = cv::Mat(pic.rows, pic.cols, CV_16S, cv::Scalar(0));
	//cout << "init done" << endl;
}

void dpx() {
	//cout << "dpx!" << endl;
	//cout << pic.rows << ' ' << pic.cols << endl;
	for (int i = 0; i < pic.rows; i++) ene.at<float>(i, 0) = gradient.at<float>(i, 0);
	for (int j = 1; j < pic.cols; j++) {
		for (int i = 0; i < pic.rows; i++) {
			float c = gradient.at<float>(i, j);
			ene.at<float>(i, j) = ene.at<float>(i, j - 1);
			trace.at<int16_t>(i, j) = 0;
			if (i > 0 && ckmin(ene.at<float>(i, j), ene.at<float>(i - 1, j - 1)))
				trace.at<int16_t>(i, j) = -1;
			if (i < pic.rows - 1 && ckmin(ene.at<float>(i, j), ene.at<float>(i + 1, j - 1)))
				trace.at<int16_t>(i, j) = 1;
			ene.at<float>(i, j) += c;
		}
	}
	int p = 0;
	energx = ene.at<float>(0, pic.cols - 1);
	for (int i = 1; i < pic.rows; i++) {
		if (ckmin(energx, ene.at<float>(i, pic.cols - 1)))
			p = i;
	}
	cout << energx << endl;
	pathx.resize(pic.cols);
	for (int i = pic.cols - 1; i >= 0; i--) {
		pathx[i] = p;
		p += trace.at<int16_t>(p, i);
	}
	//cout << "dpx done!" << endl;
}

void dpy() {
	//cout << "dpy" << endl;
	for (int j = 0; j < pic.cols; j++) ene.at<float>(0, j) = gradient.at<float>(0, j);
	for (int i = 1; i < pic.rows; i++) {
		for (int j = 0; j < pic.cols; j++) {
			float c = gradient.at<float>(i, j);
			ene.at<float>(i, j) = ene.at<float>(i - 1, j);
			trace.at<int16_t>(i, j) = 0;
			if (j > 0 && ckmin(ene.at<float>(i, j), ene.at<float>(i - 1, j - 1)))
				trace.at<int16_t>(i, j) = -1;
			if (j < pic.cols - 1 && ckmin(ene.at<float>(i, j), ene.at<float>(i - 1, j + 1)))
				trace.at<int16_t>(i, j) = 1;
			ene.at<float>(i, j) += c;
		}
	}
	int p = 0;
	energy = ene.at<float>(pic.rows - 1, 0);
	for (int j = 1; j < pic.cols; j++) {
		if (ckmin(energy, ene.at<float>(pic.rows - 1, j)))
			p = j;
	}
	cout << energy << endl;
	pathy.resize(pic.rows);
	for (int i = pic.rows - 1; i >= 0; i--) {
		pathy[i] = p;
		p += trace.at<int16_t>(i, p);
	}
	//cout << "dpy done" << endl;
}

Pixel avg(Pixel a, Pixel b) {
	Pixel c;
	c.x = cv::saturate_cast<uint8_t>((int(a.x) + int(b.x)) / 2);
	c.y = cv::saturate_cast<uint8_t>((int(a.y) + int(b.y)) / 2);
	c.z = cv::saturate_cast<uint8_t>((int(a.z) + int(b.z)) / 2);
	return c;
}

void removex() {
	//cout << "removex!" << endl;
	Pixel color = randColor();
	cv::Mat tmpPic(pic.rows - 1, pic.cols, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat tmpMsk(pic.rows - 1, pic.cols, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat tmpPos(pic.rows - 1, pic.cols, CV_16UC2, cv::Scalar(0, 0));
	
	for (int j = 0; j < pic.cols; j++) {
		int p = pathx[j];
		for (int i = 0; i < p; i++)
			tmpPic.at<Pixel>(i, j) = pic.at<Pixel>(i, j),
			tmpPos.at<Pos>(i, j) = pos.at<Pos>(i, j),
			tmpMsk.at<Pixel>(i, j) = mask.at<Pixel>(i, j);
		process.at<Pixel>(pos.at<Pos>(p, j)) = color;
		for (int i = p; i < pic.rows - 1; i++)
			tmpPic.at<Pixel>(i, j) = pic.at<Pixel>(i + 1, j),
			tmpPos.at<Pos>(i, j) = pos.at<Pos>(i + 1, j),
			tmpMsk.at<Pixel>(i, j) = mask.at<Pixel>(i + 1, j);
		if (p > 0) tmpPic.at<Pixel>(p - 1, j) = avg(pic.at<Pixel>(p - 1, j), pic.at<Pixel>(p, j));
		if (p < pic.rows - 1) tmpPic.at<Pixel>(p, j) = avg(pic.at<Pixel>(p + 1, j), pic.at<Pixel>(p, j));
	}
	pic = tmpPic;
	pos = tmpPos;
	mask = tmpMsk;
	//cout << "removex done" << endl;
}

void removey() {
	Pixel color = randColor();
	cv::Mat tmpPic(pic.rows, pic.cols - 1, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat tmpMsk(pic.rows, pic.cols - 1, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat tmpPos(pic.rows, pic.cols - 1, CV_16UC2, cv::Scalar(0, 0));

	for (int i = 0; i < pic.rows; i++) {
		int p = pathy[i];
		for (int j = 0; j < p; j++)
			tmpPic.at<Pixel>(i, j) = pic.at<Pixel>(i, j),
			tmpPos.at<Pos>(i, j) = pos.at<Pos>(i, j),
			tmpMsk.at<Pixel>(i, j) = mask.at<Pixel>(i, j);
		process.at<Pixel>(pos.at<Pos>(i, p)) = color;
		for (int j = p; j < pic.cols - 1; j++)
			tmpPic.at<Pixel>(i, j) = pic.at<Pixel>(i, j + 1),
			tmpPos.at<Pos>(i, j) = pos.at<Pos>(i, j + 1),
			tmpMsk.at<Pixel>(i, j) = mask.at<Pixel>(i, j + 1);
		if (p > 0) tmpPic.at<Pixel>(i, p - 1) = avg(pic.at<Pixel>(i, p - 1), pic.at<Pixel>(i, p));
		if (p < pic.cols - 1) tmpPic.at<Pixel>(i, p) = avg(pic.at<Pixel>(i, p + 1), pic.at<Pixel>(i, p));
	}
	pic = tmpPic;
	pos = tmpPos;
	mask = tmpMsk;
}

void expandx() {
	//cout << "expandx!" << endl;
	Pixel color = randColor();
	cv::Mat tmpPic(pic.rows + 1, pic.cols, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat tmpPro(pic.rows + 1, pic.cols, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat tmpMsk(pic.rows + 1, pic.cols, CV_8UC3, cv::Scalar(0, 0, 0));
	for (int j = 0; j < pic.cols; j++) {
		int p = pathx[j];
		for (int i = 0; i < p; i++)
			tmpPic.at<Pixel>(i, j) = pic.at<Pixel>(i, j),
			tmpPro.at<Pixel>(i, j) = process.at<Pixel>(i, j),
			tmpMsk.at<Pixel>(i, j) = mask.at<Pixel>(i, j);
		tmpPro.at<Pixel>(p, j) = color;
		tmpMsk.at<Pixel>(p, j) = Pixel(0, 255, 0);
		if (p == pic.rows - 1) tmpPic.at<Pixel>(p, j) = pic.at<Pixel>(p, j);
		else tmpPic.at<Pixel>(p, j) = avg(pic.at<Pixel>(p, j), pic.at<Pixel>(p + 1, j));
		for (int i = p + 1; i <= pic.rows; i++)
			tmpPic.at<Pixel>(i, j) = pic.at<Pixel>(i - 1, j),
			tmpPro.at<Pixel>(i, j) = process.at<Pixel>(i - 1, j),
			tmpMsk.at<Pixel>(i, j) = mask.at<Pixel>(i - 1, j);
		tmpMsk.at<Pixel>(p + 1, j) = Pixel(0, 255, 0);
		if (p + 2 <= pic.rows) tmpMsk.at<Pixel>(p + 2, j) = Pixel(0, 255, 0);
	}
	pic = tmpPic;
	process = tmpPro;
	mask = tmpMsk;
	//cout << "expandx done" << endl;
}

void expandy() {
	//cout << "expandy!" << endl;
	Pixel color = randColor();
	cv::Mat tmpPic(pic.rows, pic.cols + 1, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat tmpPro(pic.rows, pic.cols + 1, CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Mat tmpMsk(pic.rows, pic.cols + 1, CV_8UC3, cv::Scalar(0, 0, 0));
	for (int i = 0; i < pic.rows; i++) {
		int p = pathy[i];
		for (int j = 0; j < p; j++)
			tmpPic.at<Pixel>(i, j) = pic.at<Pixel>(i, j),
			tmpPro.at<Pixel>(i, j) = process.at<Pixel>(i, j),
			tmpMsk.at<Pixel>(i, j) = mask.at<Pixel>(i, j);
		tmpPro.at<Pixel>(i, p) = color;
		tmpMsk.at<Pixel>(i, p) = Pixel(0, 255, 0);
		if (p == pic.cols - 1) tmpPic.at<Pixel>(i, p) = pic.at<Pixel>(i, p);
		else tmpPic.at<Pixel>(i, p) = avg(pic.at<Pixel>(i, p), pic.at<Pixel>(i, p + 1));
		for (int j = p + 1; j <= pic.cols; j++)
			tmpPic.at<Pixel>(i, j) = pic.at<Pixel>(i, j - 1),
			tmpPro.at<Pixel>(i, j) = process.at<Pixel>(i, j - 1),
			tmpMsk.at<Pixel>(i, j) = mask.at<Pixel>(i, j - 1);
		tmpMsk.at<Pixel>(i, p + 1) = Pixel(0, 255, 0);
		if (p + 2 <= pic.cols) tmpMsk.at<Pixel>(i, p + 2) = Pixel(0, 255, 0);
	}
	pic = tmpPic;
	process = tmpPro;
	mask = tmpMsk;
	//cout << "expandy done" << endl;
}
void scaleDownwards() {
	pic.copyTo(process);
	pos = cv::Mat(pic.rows, pic.cols, CV_16UC2, cv::Scalar(0, 0));
	pos.forEach<Pos>([&](Pos &_pos, const int p[]) -> void {
		_pos.x = p[1]; _pos.y = p[0];
	});
	for (; deltax && deltay;) {
		init();
		dpx(); dpy();
		if (energx < energy) removex(), deltax--;
		else removey(), deltay--;
		//cout << energx << ' ' << energy << endl;
	} 
	for (; deltax; deltax--) {
		init(); dpx();
		removex();
	}
	for (; deltay; deltay--) {
		init(); dpy();
		removey();
	}
}
void scaleUpwards() {
	pic.copyTo(process);
	useMask = true;
	mask = cv::Mat(pic.rows, pic.cols, CV_8UC3, cv::Scalar(0, 0, 0));
	for (; deltax; deltax--) {
		init(); dpx();
		expandx();
		cv::imshow("111", mask);
		cv::waitKey(0);
	}
	
	mask = cv::Mat(pic.rows, pic.cols, CV_8UC3, cv::Scalar(0, 0, 0));
	for (; deltay; deltay--) {
		init(); dpy();
		expandy();
		cv::imshow("111", mask);
		cv::waitKey(0);
	}
}
void Main(const string &pic_name, const string &mask_name, const string &output_name, const string &process_name, float scale)
{
	srand(1926 - 8 - 17 + time(0));
	pic = cv::imread(pic_name, CV_LOAD_IMAGE_COLOR);
	mask = cv::Mat(pic.rows, pic.cols, CV_8UC3, cv::Scalar(0, 0, 0));
	if (mask_name != "") {
		mask = cv::imread(mask_name, CV_LOAD_IMAGE_COLOR);
		useMask = true;
	}
	downwards = scale < 0;
	scale = abs(scale);
	deltax = pic.rows * scale / 100;
	deltay = pic.cols * scale / 100;
	cout << deltax << " " << deltay << endl;
	cout << pic.rows << " " << pic.cols << endl;
	cout << (downwards ? '-' : '+') << endl;
	if (downwards) scaleDownwards();
	else scaleUpwards();
	//cout << pic << endl;
	cv::imwrite(output_name, pic);
	cout << pic.rows << " " << pic.cols << endl;
	if (process_name != "") {
		cv::imwrite(process_name, process);
	}
}
