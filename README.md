# SFND 3D Object Tracking

By completing all the lessons, you now have a solid understanding of keypoint detectors, descriptors, and methods to match them between successive images. Also, you know how to detect objects in an image using the YOLO deep-learning framework. And finally, you know how to associate regions in a camera image with Lidar points in 3D space. Let's take a look at our program schematic to see what we already have accomplished and what's still missing.

<img src="images/course_code_structure.png" width="779" height="414" />

In this project, I will implement the missing parts in the schematic. To do this, I will complete four major tasks: 
1. First, I will develop a way to match 3D objects over time by using keypoint correspondences. 
2. Second, I will compute the TTC based on Lidar measurements. 
3. I will then proceed to do the same using the camera, which requires to first associate keypoint matches to regions of interest and then to compute the TTC based on those matches. 
4. And lastly, I will conduct various tests with the framework. Your goal is to identify the most suitable detector/descriptor combination for TTC estimation and also to search for problems that can lead to faulty measurements by the camera or Lidar sensor.

## II. Project Tasks


The design of the project is based on the schematic shown above. I completed the following tasks to achieve that goal.

#### 0. Match 2D objects

In the [matching2D_Student.cpp](./src/matching2D_Student.cpp), I resued the mid-term project code to detect keypoints, extract descriptors and match descriptors. Various type of detectors and descriptors are implemented.

#### 1. Match 3D Objects

In the [camFusion_Student.cpp](./src/camFusion_Student.cpp), I implemented `matchBoundingBoxes()` method, in which inputs are previous and the current data frames as well as matched keypoints between the two frames. Only those bounding boxes containing the matched keypoints will be considered, i.e. within the ROI. For each bounding box in the previous frame, only one best match in the current frame is extracted. The final output is a hashmap of bounding box matches.

#### 2. Compute TTC with Lidar data

In the [camFusion_Student.cpp](./src/camFusion_Student.cpp), `computeTTCLidar()` method was implemented. To eliminate the outliers (e.g. too close to ego vehicle), I used 20% of the total number of Lidar points to calculate the averaging closest distance to the preceding vehicle. This would be more robust than simply using the closest Lidar point.

#### 3. Associate keypiont correspondences with bounding boxes

In the [camFusion_Student.cpp](./src/camFusion_Student.cpp), `clusterKptMatchesWithROI()` method was developed to associate keypoint correspondences with bounding boxes. All keypoint matches must belong to a 3D object, simply checking whether the corresponding keypoints are within the ROI in the camera image. To have a robust TTC estimation, outliers among the matches are removed using the mean of all euclidean distances between keypoint matches.

#### 4. Compute TTC with camera images

In the [camFusion_Student.cpp](./src/camFusion_Student.cpp), I completed the `computeTTCCamera` method.

After finishing the pipeline in the [FinalProject_Camera.cpp](./src/FinalProject_Camera.cpp), I could generate TTC estimations based on both Lidar points and Camera images.

