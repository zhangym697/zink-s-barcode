#include<opencv2/opencv.hpp>
#include<iostream>
#include<numeric>
#include<fstream>
#include<zbar.h>
#include<highgui.h>
#include<time.h>
#include<math.h>
#define ERROR 0.0

using namespace std;
using namespace cv;
using namespace zbar;

const int width = 640;
const int height = 480;
const int framesize = width * height * 3 / 2;   //一副图所含的像素个数 
bool upsidedown = false;
//vector<vector<Point>> contours;
//vector<Vec4i> hierachy;

//*显示图片
void show_img(Mat &img) {
	imshow("Test", img);
	waitKey(0);
}

//*transform radian system to angle system
double degreeTrans(double theta) {
	double rst = theta / CV_PI * 180;
	return rst;
}

/**rotate the input image*/
void rotateImage(Mat &src, Mat &dst, double degree, Point2f center) {
	//Point2f center;
	//center.x = float(src.cols / 2.0);
	//center.y = float(src.rows / 2.0);
	int length = 0;
	length = sqrt(src.cols*src.cols + src.rows*src.rows);
	//计算二维旋转的仿射变换矩阵
	Mat affineMrt = getRotationMatrix2D(center, degree, 1);
	//仿射变换，背景色填充为白色
	warpAffine(src, dst, affineMrt, Size(src.cols, src.rows), 1, 0, Scalar(255, 255, 255));
}

Point2f rotatePoint(Point2f input_point,double degree,Point2f center, Mat &affineMrt) {
	Point2f coordinate;
	coordinate.x = input_point.x*affineMrt.at<double>(0, 0) + input_point.y*affineMrt.at<double>(0, 1) + affineMrt.at<double>(0, 2);
	coordinate.y = input_point.x*affineMrt.at<double>(1, 0) + input_point.y*affineMrt.at<double>(1, 1) + affineMrt.at<double>(1, 2);
	return coordinate;
}

string row_scanner(Mat &img, int row) {
	int w = img.cols;
	int h = img.rows;
	string bicode = "";
	string slre = "";
	for (int col = 0; col < w; col++) {
		if (img.at<uchar>(row, col) == 0)bicode = bicode + '0';
		else bicode = bicode + '1';
	}
	bicode += '0';
	int renum1 = 0;
	int renum0 = -1;
	char pre = bicode[0];
	for (int i = 0; i <= w; i++) {
		if (bicode[i] == pre) {
			if (bicode[i] == '1') renum1++;
			if (bicode[i] == '0') renum0--;
		}
		else {
			if (bicode[i] == '0') {
				char *temp = new char;
				sprintf(temp, "%d", renum1);
				slre += temp;
				slre += ' ';
				renum1 = 1;
				pre = bicode[i];
			}
			if (bicode[i] == '1') {
				char *temp = new char;
				sprintf(temp, "%d", renum0);
				slre += temp;
				slre += ' ';
				renum0 = -1;
				pre = bicode[i];
			}
		}
	}
	return slre;
}

int main(int argc, char* argv[]) {
	int scale = 4;
	
	Mat grayImage;
	ImageScanner scanner;
	scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);
	//Mat srcImage = imread("C:\\Users\\lenovo\\source\\repos\\Project2\\test_barcode\\rotate.png"); //电子倾斜条形码
	//Mat srcImage = imread("C:\\Users\\lenovo\\source\\repos\\Project2\\test_barcode\\rotate2.png"); //电子倾斜条形码
	//Mat srcImage = imread("C:\\Users\\lenovo\\source\\repos\\Project2\\test_barcode\\lappedbarcode.png"); //电子水平条形码
	//Mat srcImage = imread("C:\\Users\\lenovo\\source\\repos\\Project2\\test_barcode\\upsidedown.png"); //电子水平条形码
	//Mat srcImage = imread("C:\\Users\\lenovo\\source\\repos\\Project2\\test_barcode\\camera_horizontal.png"); //相机水平条形码
	//Mat srcImage = imread("C:\\Users\\lenovo\\source\\repos\\Project2\\test_barcode\\camera_rotate.png"); //相机倾斜条形码
	//string yuvpath = "C:\\Users\\lenovo\\source\\repos\\inputYUV\\ok\\4.yuv";
	string yuvpath = "test_barcode.yuv";
	FILE *fp = NULL;
	errno_t ret = fopen_s(&fp, yuvpath.c_str(), "rb");
	if (ret)
	{
		printf("file does not exist\n");
	}
	int w = 640;
	int h = 480;
	int imageLength = h * w * 3 / 2;
	Mat img_yuv = Mat(h * 3 / 2, w, CV_8UC1);

	fread(img_yuv.data, sizeof(unsigned char), h*w, fp);
	fread(img_yuv.data + h * w, sizeof(unsigned char), h*w / 4, fp);
	fread(img_yuv.data + h * w * 5 / 4, sizeof(unsigned char), h*w / 4, fp);

	fclose(fp);
	fp = NULL;

	cvtColor(img_yuv, grayImage, CV_YUV2GRAY_I420); // 后缀I420代表YUV是I420格式的

	//show_img(grayImage);
	//cvtColor(srcImage, grayImage, CV_BGR2GRAY);
	//imwrite("C:\\Users\\lenovo\\source\\repos\\Project2\\test_barcode\\grayBarcode.png", grayImage);
	clock_t start, finish;
	double duration;
	start = clock();
	/*=================================================================================================*/
	//获取条形码角点坐标并进行旋转校正
	Mat midImg, midImg1, midImg2, midImg3, dst;
	vector<vector<Point>> contours;
	resize(grayImage, midImg1, Size(640/scale, 480/scale));
	Mat kernel = getStructuringElement(MORPH_RECT, Size(21, 7));
	morphologyEx(midImg1, midImg2, MORPH_OPEN, kernel);
	//show_img(midImg2);
	Canny(midImg2, midImg3, 100, 200, 3);
	//show_img(midImg3);
	findContours(midImg3, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));
	vector<vector<Point>> buf = vector<vector<Point>>(contours.size());
	vector<RotatedRect> box(contours.size()); //定义最小外接矩形集合
	Point2f rect[4];
	for (int i = 0; i < contours.size(); i++) {
		box[i] = minAreaRect(Mat(contours[i]));  //计算每个轮廓最小外接矩形
		box[i].points(rect);  //把最小外接矩形四个端点复制给rect数组，数组第一个数据为y值最大的端点
		//for (int j = 0; j < 4; j++) {
		//	line(testImage, rect[j], rect[(j + 1) % 4], Scalar(255, 0, 0), 1, CV_AA);  //绘制最小外接矩形每条边
		//}
	}
	double angle;
	if (((rect[0].y - rect[1].y)*(rect[0].y - rect[1].y) + (rect[0].x - rect[1].x)*(rect[0].x - rect[1].x)) <
		((rect[1].y - rect[2].y)*(rect[1].y - rect[2].y) + (rect[1].x - rect[2].x)*(rect[1].x - rect[2].x))) {
		angle = atan((rect[1].y - rect[2].y) / (rect[1].x - rect[2].x));
		angle = degreeTrans(angle);
	}
	else
	{
		angle = atan((rect[0].y - rect[1].y) / (rect[0].x - rect[1].x));
		angle = degreeTrans(angle);
	}
	/*图像矫正*/
	Point2f center;
	center.x = box[0].center.x;
	center.y = box[0].center.y;
	Point2f rotatedRect[4];
	int minx = 480;
	int miny = 640;
	int maxx = 0;
	int maxy = 0;
	if (angle != 0) {
		//circle(srcImage, center*scale, 3, Scalar(255, 0, 0), -1);
		rotateImage(grayImage, dst, angle, center*scale);
		Mat affineMrt = getRotationMatrix2D(center, angle, 1);
		for (int i = 0; i < 4; i++) {
				rotatedRect[i] = rotatePoint(rect[i],angle,center,affineMrt)*scale;
			}
			for (int i = 0; i < 4; i++) {
				if (rotatedRect[i].x < minx) minx = rotatedRect[i].x;
				if (rotatedRect[i].y < miny) miny = rotatedRect[i].y;
				if (rotatedRect[i].x > maxx) maxx = rotatedRect[i].x;
				if (rotatedRect[i].y > maxy) maxy = rotatedRect[i].y;
			}
			Rect barcode(minx, miny, (maxx - minx), (maxy - miny));
			dst = dst(barcode);
			copyMakeBorder(dst, dst, 2, 2, 2, 2, BORDER_CONSTANT, Scalar(255, 255, 255));
			//show_img(dst);
	}
	else {
		dst = grayImage.clone();
		for (int i = 0; i < 4; i++) {
			rect[i] = rect[i] * scale;
			if (rect[i].x < minx) minx = rect[i].x;
			if (rect[i].y < miny) miny = rect[i].y;
			if (rect[i].x > maxx) maxx = rect[i].x;
			if (rect[i].y > maxy) maxy = rect[i].y;
		}
		//for (int j = 0; j < 4; j++) {
		//	line(dst, rect[j], rect[(j + 1) % 4], Scalar(0, 255, 0), 2, CV_AA);  //绘制最小外接矩形每条边
		//}
		Rect barcode(minx, miny, (maxx - minx), (maxy - miny));
		dst = dst(barcode);
		copyMakeBorder(dst, dst, 2, 2, 2, 2, BORDER_CONSTANT, Scalar(255, 255, 255));
		//show_img(dst);

	}
	//show_img(dst);
	/*=================================================================================================*/
	//判断正反
	Mat bidst;
	threshold(dst, bidst, 0, 255, CV_THRESH_OTSU);
	//show_img(bidst);
	string lreString = row_scanner(bidst, bidst.rows / 2);
	string temp = "";
	vector<int> lreInt;
	int lrenum = 0;
	for (int i = 0; i < lreString.size(); i++) {
		if (lreString[i] == ' ') {
			lreInt.push_back(atoi(temp.c_str()));
			temp = "";
			lrenum++;
			continue;
		}
		temp += lreString[i];
	}
	double simStart, simStop;
	simStart = abs((double)(lreInt[2] / lreInt[4] * 1.0 - 0.5));
	simStop = abs((double)(lreInt[2] / lreInt[4] * 1.0 - 1));
	if (simStart > simStop) {
		upsidedown = true;
	}
	finish = clock();
	duration = (double)(finish - start) / CLOCKS_PER_SEC * 1000;
	cout << "运行时间为：" << duration << " ms" << endl;
	//show_img(dst);
/*********************
尝试通过旋转矩阵，直接获得倾斜校正后的图像的边缘线角点坐标，并用新的角点坐标裁剪出更小的条形码图像，并利用resize进一步降低数据量，提高识别速度（已完成）
***********************/
	resize(dst, dst, Size(dst.cols / 3, dst.rows / 3));
	int width = dst.cols;
	int height = dst.rows;
	uchar *raw = (uchar *)dst.data;
	Image imageZbar(width, height, "Y800", raw, width * height);
	scanner.scan(imageZbar); //扫描条码
	Image::SymbolIterator symbol = imageZbar.symbol_begin();				//获取条形码信息，三个条形码同时
	finish = clock();
	duration = (double)(finish - start) / CLOCKS_PER_SEC * 1000;
	cout << "运行时间为：" << duration << " ms" << endl;
	string coordinate[3] = {"","",""};
	for (int i = 0; symbol != imageZbar.symbol_end(); ++symbol)
	{
		coordinate[i] = symbol->get_data();
		i++;
		//cout << "识别内容为：" << endl << symbol->get_data() << endl << endl;
	}
	string result = "";
	//if (coordinate[0][0] == '6')	upsidedown = true;
	if (!upsidedown) {
		result += coordinate[2];
		result += coordinate[1];
		result += coordinate[0];
		cout << "条码内容为：" << result << endl;
		if (angle >= 0) {
			cout << "K坐标点：" << rect[2] * scale << endl;
			cout << "Q坐标点：" << rect[3] * scale << endl;
			cout << "Y坐标点：" << rect[1] * scale << endl;
			cout << "旋转角度为" << angle << "°" << endl;

		}
		else {
			cout << "K坐标点：" << rect[1] * scale << endl;
			cout << "Q坐标点：" << rect[2] * scale << endl;
			cout << "Y坐标点：" << rect[0] * scale << endl;
			cout << "旋转角度为" << 360 + angle << "°" << endl;
		}
	}
	else {
		result += coordinate[0];
		result += coordinate[1];
		result += coordinate[2];
		cout << "条码内容为：" << result << endl;
		if (angle >= 0) {
			cout << "K坐标点：" << rect[0] * scale << endl;
			cout << "Q坐标点：" << rect[1] * scale << endl;
			cout << "Y坐标点：" << rect[3] * scale << endl;
			cout << "旋转角度为" << angle << "°" << endl;
		}
		else {
			cout << "K坐标点：" << rect[3] * scale << endl;
			cout << "Q坐标点：" << rect[0] * scale << endl;
			cout << "Y坐标点：" << rect[2] * scale << endl;
			cout << "旋转角度为" << 180 + angle << "°" << endl;
		}
	}
	if (upsidedown) {
		cout << "校正后条码为反向" << endl;
	}
	else if(!upsidedown){
		cout << "校正后条码为正向" << endl;
	}
	imageZbar.set_data(NULL, 0);
	return 0;
}