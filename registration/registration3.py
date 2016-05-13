import PIL
from PIL import Image
import pylab
import numpy as np
import cv2
import matplotlib.pyplot as plt
import scipy as sp



# Open image
image1 = Image.open("C:\\Users\\Rebecka\\Documents\\Computational Photography\\Samples\\tests\\city\\01_rgb.tiff")
rgb = np.array(image1.convert("L"))
print(rgb)

image2 = Image.open("C:\\Users\\Rebecka\\Documents\\Computational Photography\\Samples\\tests\\city\\01_nir.tiff")
nir = np.array(image2.convert("L"))

# Detect keypoint using SIFT
detector =cv2.SIFT(0, 3, 0.08, 10, 1.6)
det_rgb = detector.detect(rgb)
det_nir = detector.detect(nir)

print '#keypoints in rgb_image: %d, nir_image: %d' % (len(det_rgb), len(det_nir))

# Calculate descriptors (feature vectors)
extractor =cv2.DescriptorExtractor_create("SIFT")

k1, d1 = extractor.compute(rgb, det_rgb)
k2, d2 = extractor.compute(nir, det_nir)


# Match the keypoints
matcher =cv2.BFMatcher() #MBFatcher object
matchedKeypoints = matcher.match(d1, d2)

#-----

FLANN_INDEX_KDTREE = 0
index_params = dict(algorithm = FLANN_INDEX_KDTREE, trees = 5)
search_params = dict(checks = 50)

flann = cv2.FlannBasedMatcher(index_params, search_params)

matches = flann.knnMatch(d1,d2,k=2)

# store all the good matches as per Lowe's ratio test.
good = []
for m,n in matches:
    if m.distance < 0.7*n.distance:
        good.append(m)


MIN_MATCH_COUNT = 3

if len(good)>MIN_MATCH_COUNT:
    src_pts = np.float32([ k1[m.queryIdx].pt for m in good ]).reshape(-1,1,2)
    dst_pts = np.float32([ k2[m.trainIdx].pt for m in good ]).reshape(-1,1,2)

    M, mask = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC,5.0)
    matchesMask = mask.ravel().tolist()

    h,w = rgb.shape
    pts = np.float32([ [0,0],[0,h-1],[w-1,h-1],[w-1,0] ]).reshape(-1,1,2)
    dst = cv2.perspectiveTransform(pts,M)

    nir = cv2.polylines(nir,[np.int32(dst)],True,255,3)
    #    nir = cv2.polylines(nir,[np.int32(dst)],True,255,3, cv2.LINE_AA)

else:
    print "Not enough matches are found - %d/%d" % (len(good),MIN_MATCH_COUNT)
    matchesMask = None

draw_params = dict(matchColor = (0,255,0), # draw matches in green color
                   singlePointColor = None,
                   matchesMask = matchesMask, # draw only inliers
                   flags = 2)



def drawMatches(img1, kp1, img2, kp2, matches):
 
    rows1 = img1.shape[0]
    cols1 = img1.shape[1]
    rows2 = img2.shape[0]
    cols2 = img2.shape[1]

    out = np.zeros((max([rows1,rows2]),cols1+cols2,3), dtype='uint8')

    out[:rows1,:cols1,:] = np.dstack([img1, img1, img1])

    out[:rows2,cols1:cols1+cols2,:] = np.dstack([img2, img2, img2])

    for mat in matches:

        img1_idx = mat.queryIdx
        img2_idx = mat.trainIdx

        (x1,y1) = kp1[img1_idx].pt
        (x2,y2) = kp2[img2_idx].pt

        cv2.circle(out, (int(x1),int(y1)), 4, (255, 0, 0), 1)   
        cv2.circle(out, (int(x2)+cols1,int(y2)), 4, (255, 0, 0), 1)

        cv2.line(out, (int(x1),int(y1)), (int(x2)+cols1,int(y2)), (255, 0, 0), 1)

    cv2.imshow('Matched Features', out)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

#img3 = cv2.drawMatches(rgb,k1,nir,k2,None)

#plt.imshow(img3, 'gray'),plt.show()