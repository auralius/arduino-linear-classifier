# arduino-linear-classifier

This is a demonstration for the linear classifiers (SVM and softmax) described in : https://github.com/auralius/AEK3LBB3 or https://auralius.github.io/AEK3LBB3/intro.html

The classifier is used to classifiy handwritten digits as explained in https://archive.ics.uci.edu/dataset/80/optical+recognition+of+handwritten+digits.

![](./demo.gif)

The obtained weight matrix (`W`) from [AEK3LBB3](https://github.com/auralius/AEK3LBB3) is then plugged into the classifier program inside the Arduino device (in this case: Mega2560). The classifier program is mainly composed of:  
* acquisition
* discretization
* matrix multiplication (which is the predict part of the classifier)

**Note:**

There is no image-size normalization applied yet. Thus, it is important to draw the digits as large as possible.
