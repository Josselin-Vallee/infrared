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
detector =cv2.SIFT()
det_rgb = detector.detect(rgb)
det_nir = detector.detect(nir)

extractor =cv2.DescriptorExtractor_create("SIFT")
#desc = extractor.compute(rgb,feat)
#desc_nir = extractor.compute(nir,feat_nir)

k1, d1 = extractor.compute(rgb,det_rgb)
k2, d2 = extractor.compute(nir,det_nir)

img1 = cv2.drawKeypoints (rgb, det_rgb, color = (255,0,0))
cv2.imwrite('image1.png',img1)
#matching points in the image
#BF - brute force
matcher =cv2.BFMatcher()
#MFMatcher.match returns the best match
matchedImage = matcher.match(d1,d2)
print k1
print d1

#creating list for distance between keypoints
print '#matchedImage:', len(matchedImage)
dist = [m.distance for m in matchedImage]

print 'distance: min: %.3f' % min(dist)
print 'distance: mean: %.3f' % (sum(dist) / len(dist))
print 'distance: max: %.3f' % max(dist)

#good = []
#for m,n in dist:
#    if m.distance < 0.75*n.distance:
#        good.append([m])
#print good


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
