import infrared.shadow.shadow_detection as sd
import matplotlib.pyplot as plt
from scipy import misc

rgb = misc.imread("img_68_vis.png")
nir = misc.imread("img_68_nir.png")

Ubin =sd.shadowDetection(rgb, nir)

plt.imshow(Ubin, cmap='Greys_r')

plt.show()
