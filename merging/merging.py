import cv2
import numpy as np

def merge(rgb, nir)
	#import RGB image
	rgb = cv2.imread(rgb, 3)
	#import NIR image
	nir = cv2.imread(nir, 0)
	#Convert RGB image into YCbCr
	ycc = cv2.cvtColor(rgb, cv2.COLOR_BGR2YCR_CB)
	#Split the Y, Cb, Cr channels :
	y, cb, cr = cv2.split(ycc)

	#compute mean of y image
	yMean = cv2.mean(y)
	#compute mean of nir image
	nirMean = cv2.mean(nir)
	#compute mean shift
	shift = yMean[0] - nirMean[0]
	#apply shift to nir image
	nir = cv2.add(nir, shift)
	#apply bilateral filter to get the base layers :
	d = 30
	sigmaC = 15
	sigmaS = 5
	nirBase = cv2.bilateralFilter(nir, d, sigmaC, sigmaS)
	yBase = cv2.bilateralFilter(y, d, sigmaC, sigmaS)

	#get detail layer for nir image:
	nirDetail = cv2.subtract(nir,nirBase, -1)

	#add the 2 layers
	outY = cv2.add(nirDetail,yBase, -1)
	#add the chrominance information:
	out = cv2.merge((outY, cb, cr))
	#convert to RGB
	out = cv2.cvtColor(out, cv2.COLOR_YCR_CB2BGR)

	return out

#Display original image and output
# cv2.imshow('Output', out)
# cv2.imshow('Original', rgb)
#Display NIR image
# cv2.waitKey(0)

