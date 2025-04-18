# arduino-linear-classifier

This is a demonstration for the linear classifiers (SVM and softmax) described in https://github.com/auralius/AEK3LBB3 or https://auralius.github.io/AEK3LBB3/intro.html.

The classifier is used to classify handwritten digits as explained in https://archive.ics.uci.edu/dataset/80/optical+recognition+of+handwritten+digits.

 <img src="./demo.gif" alt="Description" width="400">

Devices used:

* Tested on:

    * Arduino UNO R3
    
    * Arduino Mega2560 
    
    * Arduino UNO R4 WiFi or UNO R4 Minima

* 2.4" TFT-LCD with touch from MCUFRIEND_kbv ([purchase link](https://www.tokopedia.com/cncstorebandung/cnc-2-4-tft-lcd-touch-shield-module-for-arduino-uno-mega))

The obtained weight matrix Wfrom [AEK3LBB3](https://github.com/auralius/AEK3LBB3) is then plugged into the classifier program inside the Arduino device. The classifier program is mainly composed of

* acquisition

* discretization, position- and size-normalization

* matrix multiplication (which is the **prediction** part of the classifier)

Note:

* This implementation has been tested on Arduino Uno R3, R4, and Mega2560.  However, special actions are needed for Arduino UNO R4. To make the LCD works with an Arduino Uno R4, instead of using the default MCUFRIEND_kbv library, use the alternative library from this [link.](https://github.com/slviajero/MCUFRIEND_kbv)
