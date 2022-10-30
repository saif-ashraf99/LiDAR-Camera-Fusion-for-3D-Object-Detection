#include <numeric>
#include "matching2D.hpp"

using namespace std;

// Find best matches for keypoints in two camera images based on several matching methods
void matchDescriptors(std::vector<cv::KeyPoint>& kPtsSource, std::vector<cv::KeyPoint>& kPtsRef, cv::Mat& descSource, cv::Mat& descRef,
    std::vector<cv::DMatch>& matches, std::string descriptorType, std::string matcherType, std::string selectorType)
{
    // configure matcher
    bool crossCheck = false;
    cv::Ptr<cv::DescriptorMatcher> matcher;
    double t;

    //if brute force
    if (matcherType.compare("MAT_BF") == 0)
    {
        int normType = descriptorType.compare("DES_BINARY") == 0 ? cv::NORM_HAMMING : cv::NORM_L2;
        matcher = cv::BFMatcher::create(normType, crossCheck);
        cout << "BF matching cross-check = " << crossCheck;
    }

    else if (matcherType.compare("MAT_FLANN") == 0)
    {
        if (descSource.type() != CV_32F || descRef.type() != CV_32F)
        {
            descSource.convertTo(descSource, CV_32F);
            descRef.convertTo(descRef, CV_32F);
        }

        matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
        cout << "FLANN matching";
    }

    // perform matching task
    if (selectorType.compare("SEL_NN") == 0)
    { // nearest neighbor (best match)
        t = (double)cv::getTickCount();
        matcher->match(descSource, descRef, matches); // Finds the best match for each descriptor in desc1
        t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
        cout << " NN with n= " << matches.size() << " matches in " << 1000 * t / 1.0 << " ms" << endl;
    }
    // k nearest neighbors (k=2)
    else if (selectorType.compare("SEL_KNN") == 0)
    {
        vector<vector<cv::DMatch>> knn_matches;
        t = (double)cv::getTickCount();
        matcher->knnMatch(descSource, descRef, knn_matches, 2); // Finds the 2 best match 
        t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
        cout << " KNN with n=" << knn_matches.size() << " matches in " << 1000 * t / 1.0 << " ms" << endl;

        // filter matches using descriptor distance ratio test
        const double minDescDistRatio = 0.8f;
        for (int i = 0; i < knn_matches.size(); i++)
        {
            if (knn_matches[i][0].distance < minDescDistRatio * knn_matches[i][1].distance)
            {
                matches.push_back(knn_matches[i][0]);
            }
        }
        cout << "Distance ratio test removed " << knn_matches.size() - matches.size() << " keypoints" << endl;
    }
}

// Use one of several types of state-of-art descriptors to uniquely identify keypoints
int descKeypoints(vector<cv::KeyPoint>& keypoints, cv::Mat& img, cv::Mat& descriptors, string descriptorType)
{
    // select appropriate descriptor
    cv::Ptr<cv::DescriptorExtractor> extractor;
    if (descriptorType.compare("BRISK") == 0)
    {

        int threshold = 30;        // FAST/AGAST detection threshold score.
        int octaves = 3;           // detection octaves (use 0 to do single scale)
        float pattern_scale = 1.0f; // apply this scale to the pattern used for sampling the neighbourhood of a keypoint.

        extractor = cv::BRISK::create(threshold, octaves, pattern_scale);
    }
    else if (descriptorType.compare("BRIEF") == 0)
    {
        int bytes = 32;
        bool bOrientation = false;
        extractor = cv::xfeatures2d::BriefDescriptorExtractor::create(bytes, bOrientation);
    }
    else if (descriptorType == "BRIEF") {
        extractor = cv::xfeatures2d::BriefDescriptorExtractor::create();
    }
    else if (descriptorType == "ORB") {
        extractor = cv::ORB::create();
    }
    else if (descriptorType == "FREAK") {
        extractor = cv::xfeatures2d::FREAK::create();
    }
    else if (descriptorType == "AKAZE") {
        extractor = cv::AKAZE::create();
    }
    else if (descriptorType == "SIFT") {
        extractor = cv::xfeatures2d::SIFT::create();
    }
    else {
        return 0;
    }

    // perform feature description
    double t = (double)cv::getTickCount();
    extractor->compute(img, keypoints, descriptors);
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    cout << descriptorType << " descriptor extraction in " << 1000 * t / 1.0 << " ms" << endl;
}

// Detect keypoints in image using the traditional Shi-Thomasi detector
void detKeypointsShiTomasi(vector<cv::KeyPoint>& keypoints, cv::Mat& img, bool bVis)
{
    // compute detector parameters based on image size
    int blockSize = 4;       //  size of an average block for computing a derivative covariation matrix over each pixel neighborhood
    double maxOverlap = 0.0; // max. permissible overlap between two features in %
    double minDistance = (1.0 - maxOverlap) * blockSize;
    int maxCorners = img.rows * img.cols / max(1.0, minDistance); // max. num. of keypoints

    double qualityLevel = 0.01; // minimal accepted quality of image corners
    double k = 0.04;

    // Apply corner detection
    double t = (double)cv::getTickCount();
    vector<cv::Point2f> corners;
    cv::goodFeaturesToTrack(img, corners, maxCorners, qualityLevel, minDistance, cv::Mat(), blockSize, false, k);

    // add corners to result vector
    for (auto it = corners.begin(); it != corners.end(); ++it)
    {

        cv::KeyPoint newKeyPoint;
        newKeyPoint.pt = cv::Point2f((*it).x, (*it).y);
        newKeyPoint.size = blockSize;
        keypoints.push_back(newKeyPoint);
    }
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    cout << "Shi-Tomasi detection with n=" << keypoints.size() << " keypoints in " << 1000 * t / 1.0 << " ms" << endl;

    // visualize results
    if (bVis)
    {
        cv::Mat visImage = img.clone();
        cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        string windowName = "Shi-Tomasi Corner Detector Results";
        cv::namedWindow(windowName, 6);
        imshow(windowName, visImage);
        cv::waitKey(0);
    }
}

// Detect keypoints in image using Harris detector
double  detKeypointsHarris(std::vector<cv::KeyPoint>& keypoints, cv::Mat& img, bool bVis) {
    int block_size = 2;
    int aperture_size = 3;
    int min_response = 100;
    double k = 0.04;

    // Detect Harris corners and normalize output
    cv::Mat dst, dst_norm, dst_norm_scaled;
    dst = cv::Mat::zeros(img.size(), CV_32FC1);
    double t = (double)cv::getTickCount();
    cv::cornerHarris(img, dst, block_size, aperture_size, k, cv::BORDER_DEFAULT);
    cv::normalize(dst, dst_norm, 0, 255, cv::NORM_MINMAX, CV_32FC1);
    cv::convertScaleAbs(dst_norm, dst_norm_scaled);

    // perform non-maximum suppression
    int nms_window_size = 2 * aperture_size + 1;
    int rows = dst_norm.rows;
    int cols = dst_norm.cols;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int centre_r = -1;
            int centre_c = -1;

            // The max value is set to the minimum response
            // We should have keypoints that exceed this first
            unsigned char max_val = static_cast<unsigned char>(min_response);
            for (int x = -nms_window_size; x <= nms_window_size; x++) {
                for (int y = -nms_window_size; y <= nms_window_size; y++) {
                    if ((i + x) < 0 || (i + x) >= rows) { continue; }
                    if ((j + y) < 0 || (j + y) >= cols) { continue; }
                    const unsigned char val =
                        dst_norm_scaled.at<unsigned char>(i + x, j + y);
                    if (val > max_val) {
                        max_val = val;
                        centre_r = i + x;
                        centre_c = j + y;
                    }
                }
            }

            // If the largest value was at the centre, remember this keypoint
            if (centre_r == i && centre_c == j) {
                keypoints.emplace_back(j, i, 2 * aperture_size, -1, max_val);
            }
        }
    }

    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    cout << "Harris detection with n=" << keypoints.size() << " keypoints in "
        << 1000 * t / 1.0 << " ms" << endl;

    // visualize results
    if (bVis) {
        cv::Mat visImage = img.clone();
        cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1),
            cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        string windowName = "Harris Corner Detector Results";
        cv::namedWindow(windowName, 6);
        imshow(windowName, visImage);
        cout << "Press any key to continue\n";
        cv::waitKey(0);
    }
    return t;
}

double detKeypointsModern(vector<cv::KeyPoint>& keypoints, cv::Mat& img, std::string detectorType, bool bVis)
{
    cv::Ptr<cv::FeatureDetector> detector;

    if (detectorType.compare("FAST") == 0)
    {
        int threshold = 30; // difference between intensity of the central pixel and pixels of a circle around this pixel
        bool bNMS = true;
        cv::FastFeatureDetector::DetectorType type = cv::FastFeatureDetector::TYPE_9_16;

        detector = cv::FastFeatureDetector::create(threshold, bNMS, type);
    }
    else if (detectorType.compare("BRISK") == 0)
    {
        int threshold = 30;
        int octaves = 3;
        float patterScale = 1.0f;

        detector = cv::BRISK::create(threshold, octaves, patterScale);
    }
    else if (detectorType.compare("ORB") == 0)
    {
        int nfeatures = 500;
        float scaleFactor = 1.2f;
        int nlevels = 8;
        int edgeThreshold = 31;
        int firstLevel = 0;
        int WTA_K = 2;
        cv::ORB::ScoreType scoreType = cv::ORB::HARRIS_SCORE;
        int patchSize = 31;
        int fastThreshold = 20;

        detector = cv::ORB::create(nfeatures, scaleFactor, nlevels, edgeThreshold, firstLevel, WTA_K, scoreType, patchSize, fastThreshold);
    }
    else if (detectorType.compare("AKAZE") == 0)
    {
        cv::AKAZE::DescriptorType descriptorType = cv::AKAZE::DESCRIPTOR_MLDB;
        int descriptorSize = 0;
        int descriptorChannels = 3;
        float threshold = 0.001f;
        int nOctaves = 4;
        int nOctaveLayers = 4;
        cv::KAZE::DiffusivityType diffusivity = cv::KAZE::DIFF_PM_G2;

        detector = cv::AKAZE::create(descriptorType, descriptorSize, descriptorChannels, threshold, nOctaves, nOctaveLayers, diffusivity);
    }
    else if (detectorType.compare("SIFT") == 0)
    {
        int nfeatures = 0;
        int nOctaveLayers = 3;
        double contrastThreshold = 0.04;
        double edgeThreshold = 10.0;
        double sigma = 1.6;

        detector = cv::xfeatures2d::SIFT::create(nfeatures, nOctaveLayers, contrastThreshold, edgeThreshold, sigma);
    }

    double t = (double)cv::getTickCount();
    detector->detect(img, keypoints);
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    cout << detectorType << " detector with n=" << keypoints.size() << " keypoints in " << 1000 * t / 1.0 << " ms" << endl;

    string windowName = detectorType + " Detector Results";

    // visualize results
    if (bVis)
    {
        cv::Mat visImage = img.clone();
        cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        cv::namedWindow(windowName, 6);
        imshow(windowName, visImage);
        cv::waitKey(0);
    }
    return t;
}