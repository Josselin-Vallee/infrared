

# import PIL
# from PIL import Image
import numpy
import cv2
# import matplotlib.pyplot as plt

def register(rgb, nir):
    # Open image and convert it to byte images (instead of RGB)
    # Array of arrays with pixel values 0-255
    # First element (array) is all the pixels in the first row of the image (1024 px)
    # image1 = Image.open(rgb)
    # rgb = numpy.array(image1.convert("L"))

    # image2 = Image.open(nir)
    # nir = numpy.array(image2.convert("L"))


    #import RGB image
    rgb = cv2.imread(rgb, 3)
    #import NIR image
    nir = cv2.imread(nir, 3)
    #Convert RGB image into YCbCr
    rgb = cv2.cvtColor(rgb, cv2.COLOR_BGR2YCR_CB)
    nir = cv2.cvtColor(nir, cv2.COLOR_BGR2YCR_CB)

    # Detect keypoint using SIFT
    detectKP =cv2.SIFT(0, 3, 0.08, 10, 1.6)
    kp_rgb = detectKP.detect(rgb)
    kp_nir = detectKP.detect(nir)

    #print '#keypoints in rgb_image: %d, nir_image: %d' % (len(kp_rgb), len(kp_nir))

    # Algorithm to find descriptors - surroundings of keypoints
    extractor =cv2.DescriptorExtractor_create("SIFT")

    # Getting keypoints and descriptors (list of 2 dimensions)
    k1, des1 = extractor.compute(rgb, kp_rgb)
    k2, des2 = extractor.compute(nir, kp_nir)

    # Nearest neighbor search, matching keypoints
    FLANN_INDEX_KDTREE = 0
    index_params = dict(algorithm = FLANN_INDEX_KDTREE, trees = 5)
    search_params = dict(checks = 50)

    flann = cv2.FlannBasedMatcher(index_params, search_params)
    matches = flann.knnMatch(des1,des2,k=2)

    # Ratio test
    good = []
    for m,n in matches:
        if m.distance < 0.7*n.distance:
            good.append(m)

    src_pts = numpy.float32([ k1[m.queryIdx].pt for m in good ]).reshape(-1,1,2)
    dst_pts = numpy.float32([ k2[m.trainIdx].pt for m in good ]).reshape(-1,1,2)

    M, mask = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC,5.0)

    # Warp source image to destination based on homography
    im_out = cv2.warpPerspective(rgb, M, (nir.shape[1],nir.shape[0]))

    #convert to RGB
    im_out = cv2.cvtColor(im_out, cv2.COLOR_YCR_CB2BGR)

    return im_out

out = register("lake_rgb.tiff", "lake_nir.tiff")
cv2.imwrite("output.jpg", out)
     
# Display images
# cv2.imshow("Source Image", rgb)
# cv2.imshow("Destination Image", nir)
# cv2.imshow("Warped Source Image", im_out)

# cv2.waitKey(0)
#print M, mask





#h, status = cv2.findHomography(rgb, nir)
#im_dst = cv2.warpPerspective(rgb, h, size)
#def match_and_draw(win):
#        with Timer('matching'):
#            raw_matches = matcher.knnMatch(desc1, trainDescriptors = desc2, k = 2) #2
#        p1, p2, kp_pairs = filter_matches(kp1, kp2, raw_matches)
#        if len(p1) >= 4:
#            H, status = cv2.findHomography(p1, p2, cv2.RANSAC, 5.0)
#            print '%d / %d  inliers/matched' % (np.sum(status), len(status))
#            # do not draw outliers (there will be a lot of them)
#            kp_pairs = [kpp for kpp, flag in zip(kp_pairs, status) if flag]
#        else:
#            H, status = None, None
#            print '%d matches found, not enough for homography estimation' % len(p1)
#
#        vis = explore_match(win, img1, img2, kp_pairs, None, H)



#(homography,mask) = cv2.findHomography(rgb,nir,cv2.RANSAC, ransacReprojThreshold=1.0 )
#homography = cv2.findHomography(rgb, nir[method[, ransac
#   ReprojThreshold])
#homography = cv2.findHomography(rgb,nir, method = 0, ransacReprojThreshold=3.0)
#im_dst = cv2.warpPerspective(image1, h, size)



#print '#matchedImage:', len(matchedImage)
#dist = [m.distance for m in matchedImage]

#print 'distance: min: %.3f' % min(dist)
#print 'distance: mean: %.3f' % (sum(dist) / len(dist))
#print 'distance: max: %.3f' % max(dist)

# Remove outliers

# keep only the reasonable matches
# selected_matches = [m for m in matchedImage if m.distance < threshold_dist]

# print '#selected matches:', len(selected_matches)