#import Python Imaging Library
import PIL 
from PIL import Image
import pylab
import numpy as np
import cv2
import matplotlib.pyplot as plt

#opening the images and saving in arrays
image = Image.open("C:\\Users\\Rebecka\\Documents\\Computational Photography\\Samples\\tests\\city\\01_rgb.tiff")
rgb = np.array(image.convert("L"))

image = Image.open("C:\\Users\\Rebecka\\Documents\\Computational Photography\\Samples\\tests\\city\\01_nir.tiff")
nir = np.array(image.convert("L"))

#finding the keypoints
#detector =cv2.SIFT()
#feat = detector.detect(rgb)
#feat_nir = detector.detect(nir)

#extractor =cv2.DescriptorExtractor_create("SIFT")
#desc = extractor.compute(rgb,feat)
#desc_nir = extractor.compute(nir,feat_nir)

#print desc[1:5]
#matching points in the image
#BF - brute force
#matcher =cv2.BFMatcher()
#MFMatcher.match returns the best match

#matchedImage = matcher.match(desc[1:2], desc_nir[1:2])
#print matchedImage[1:5]

#----------------------------------------------------------------
MIN_MATCH_COUNT = 10

# Initiate SIFT detector
sift = cv2.SIFT()

# find the keypoints and descriptors with SIFT
kp1, des1 = sift.detectAndCompute(rgb,None)
kp2, des2 = sift.detectAndCompute(nir,None)

FLANN_INDEX_KDTREE = 0
index_params = dict(algorithm = FLANN_INDEX_KDTREE, trees = 5)
search_params = dict(checks = 50)

flann = cv2.FlannBasedMatcher(index_params, search_params)
matches = flann.knnMatch(des1,des2,k=2)

# store all the good matches as per Lowe's ratio test.
good = []
for m,n in matches:
    if m.distance < 0.7*n.distance:
        good.append(m)

print good
#--------------------------------------------------------------------------

# if len(good)>MIN_MATCH_COUNT:
#     src_pts = np.float32([ kp1[m.queryIdx].pt for m in good ]).reshape(-1,1,2)
#     dst_pts = np.float32([ kp2[m.trainIdx].pt for m in good ]).reshape(-1,1,2)

#     M, mask = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC,5.0)
#     matchesMask = mask.ravel().tolist()

#     h,w = rgb.shape
#     pts = np.float32([ [0,0],[0,h-1],[w-1,h-1],[w-1,0] ]).reshape(-1,1,2)
#     dst = cv2.perspectiveTransform(pts,M)

#     nir = cv2.polylines(nir,[np.int32(dst)],True,255,3, cv2.CV_AA)

# else:
#     print "Not enough matches are found - %d/%d" % (len(good),MIN_MATCH_COUNT)
#     matchesMask = None  

# draw_params = dict(matchColor = (0,255,0), # draw matches in green color
#                    singlePointColor = None,
#                    matchesMask = matchesMask, # draw only inliers
#                    flags = 2)

#img3 = cv2.drawMatches(rgb,kp1,nir,kp2,good,None,**draw_params)

#plt.imshow(img3, 'gray'),plt.show()      


#showing the images next to each other
fig = plt.figure("title")

ax = fig.add_subplot(1, 2, 1)
pylab.imshow(rgb)
plt.axis("off")

ax = fig.add_subplot(1, 2, 2)
pylab.imshow(nir)
plt.axis("off")

#print numpy.shape(rgb)
#print numpy.shape(nir)
#print numpy.shape(matchedImage)
#print matchedImage[1:5]
pylab.show()
