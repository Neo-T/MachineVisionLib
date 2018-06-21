#include "stdafx.h"
#include <iostream>  
#include <fstream> 
#include <list>  
#include <vector>  
#include <map>  
#include <stack>  
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/objdetect.hpp>

using namespace std;
using namespace cv;

//* 特征点匹配最直观的说明
//* https://blog.csdn.net/civiliziation/article/details/38370167
//* cv2和cv3函数改变的：
//* https://www.cnblogs.com/anqiang1995/p/7398218.html
bool refineMatchesWithHomography(
	const std::vector<cv::KeyPoint>& queryKeypoints,
	const std::vector<cv::KeyPoint>& trainKeypoints,
	float reprojectionThreshold, std::vector<cv::DMatch>& matches,
	cv::Mat& homography) {
	const int minNumberMatchesAllowed = 8;

	if (matches.size() < minNumberMatchesAllowed)
		return false;

	// Prepare data for cv::findHomography
	std::vector<cv::Point2f> srcPoints(matches.size());
	std::vector<cv::Point2f> dstPoints(matches.size());

	for (size_t i = 0; i < matches.size(); i++) {
		srcPoints[i] = trainKeypoints[matches[i].trainIdx].pt;
		dstPoints[i] = queryKeypoints[matches[i].queryIdx].pt;
	}

	// Find homography matrix and get inliers mask
	std::vector<unsigned char> inliersMask(srcPoints.size());
	homography = cv::findHomography(srcPoints, dstPoints, CV_FM_RANSAC,
		reprojectionThreshold, inliersMask);

	std::vector<cv::DMatch> inliers;
	for (size_t i = 0; i < inliersMask.size(); i++) {
		if (inliersMask[i])
			inliers.push_back(matches[i]);
	}

	matches.swap(inliers);
	return matches.size() > minNumberMatchesAllowed;
}

void KeyPointsToPoints(vector<KeyPoint> kpts, vector<Point2f> &pts) {
	for (int i = 0; i < kpts.size(); i++) {
		pts.push_back(kpts[i].pt);
	}

	return;
}


//* 重要，能检测运动目标
//* https://www.jpjodoin.com/urbantracker/index.htm
int main(int argc, char** argv)
{
	Mat matSrcImg = imread(argv[1], IMREAD_GRAYSCALE);
	pyrDown(matSrcImg, matSrcImg, Size(matSrcImg.cols / 2, matSrcImg.rows / 2));
	vector<KeyPoint> key_points;
	Mat descriptor;
	Ptr<Feature2D> orb = ORB::create(500);
	Ptr<cv::DescriptorExtractor> extractor = xfeatures2d::FREAK::create(true, true);
	orb->detect(matSrcImg, key_points);
	extractor->compute(matSrcImg, key_points, descriptor);

	Mat matSrcImg2 = imread(argv[2], IMREAD_GRAYSCALE);
	pyrDown(matSrcImg2, matSrcImg2, Size(matSrcImg2.cols / 2, matSrcImg2.rows / 2));
	vector<KeyPoint> key_points2;
	Mat descriptor2;
	Ptr<Feature2D> orb2 = ORB::create(500);
	Ptr<cv::DescriptorExtractor> extractor2 = xfeatures2d::FREAK::create(true, true);
	orb2->detect(matSrcImg2, key_points2);
	extractor2->compute(matSrcImg2, key_points2, descriptor2);

	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::BRUTEFORCE_HAMMING);

	vector<DMatch> matches;
	Mat homography, matDstImg;

	matcher->match(descriptor2, descriptor, matches);

	refineMatchesWithHomography(key_points2, key_points, 5, matches, homography);
	drawMatches(matSrcImg2, key_points2, matSrcImg, key_points, matches, matDstImg);
	imshow("匹配图", matDstImg);

	cv::waitKey(0);
	destroyAllWindows();

	return 0;
}