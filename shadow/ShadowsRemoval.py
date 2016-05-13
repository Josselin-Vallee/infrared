from scipy import misc
import numpy as np
import matplotlib.pyplot as plt

# Parameters for the non-linear mapping
alpha = 14
beta = 0.5
gamma = 2.2


def nonlinearmapping(x):
    # This apply the non-linear mapping to x
    result = 1.0 / (1.0 + np.exp(- alpha * ((1 - x) - beta)))
    return result


def normalize(i):
    # Convert the image in the range [0;1]
    minim = np.amin(i)
    maxim = np.amax(i)
    return (i - minim) / (maxim - minim)

def im2double(i):
    # Convert the image from uint8 to double
    return i.astype('double') / 255


rgb = im2double(misc.imread("linear_images/img_68_vis.png"))
nir = im2double(misc.imread("linear_images/img_68_nir.png"))

# Extract and normalize the 3 color channels of the image
rImage = normalize((rgb[:, :, 0]))
gImage = normalize(rgb[:, :, 1])
bImage = normalize(rgb[:, :, 2])


# Compute the brightness of the RGB image
brightness = (rImage + gImage + bImage) / 3

# Non-linear mapping
f = np.vectorize(nonlinearmapping)

# We normalize the nir image
nir = normalize(nir)

# We apply the gamma correction and compute the temporary dark map dVIS and dNIR
brightness = np.power(brightness, (1.0/gamma))
dVIS = f(brightness)
nir = np.power(nir, (1.0/gamma))
dNIR = f(nir)

# We get the shadow candidate map D
D = np.multiply(dVIS, dNIR)

# We calculate the tks by dividing the color channels by the NIR ones. The 0.000001 padding is to prevent division by 0
tRed = np.divide(rImage, nir + 0.0000001)
tGreen = np.divide(gImage, nir + 0.0000001)
tBlue = np.divide(bImage, nir + 0.0000001)

# Tao is the parameter to upper bound the values t
tao = 10

# We compute the color to NIR ratio map
T = (1 / tao) * np.minimum(np.maximum.reduce([tRed, tGreen, tBlue]), tao)

# The shadow map U
U = np.multiply((1 - D), (1 - T))

nabla = 1.6
width = U.shape[0]
height = U.shape[1]
nbins = (nabla * np.ceil(np.log2(width*height) + 1))

# Boolean that determines when a valley has been found
valleyFound = False

# We find the first valley according to the histogram of the shadow mask
while not valleyFound:
    hist, bins = np.histogram(U, np.floor(nbins).astype('int'))
    valleyHeight = np.amax(hist)
    for x in range(3, (np.floor(nbins)-2).astype('int')):
        if hist[x] < valleyHeight and hist[x] < hist[x-1] < hist[x-2] and hist[x] < hist[x+1] < hist[x+2]:
            valleyHeight = hist[x]
            valleyValue = (bins[1] - bins[0]) * (x+0.5)
            valleyFound = True
    nbins += 1

Ubin = np.zeros(U.shape)
index = U[:, :] <= valleyValue
Ubin[index] = 1

fig = plt.figure(3)

a = fig.add_subplot(1, 1, 1)
plt.imshow(Ubin, cmap='Greys_r')
a.set_title('Ubin binary shadow mask')

plt.show()
