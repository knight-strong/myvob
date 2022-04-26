#!/usr/bin/python3

import numpy as np
import cv2
import sys


#flags = [i for i in dir(cv2) if i.startswith('COLOR_')]
#print(flags)

def mergeUV(u, v):
    if u.shape == v.shape:
        uv = np.zeros(shape=(u.shape[0], u.shape[1]*2))
        for i in range(0, u.shape[0]):
            for j in range(0, u.shape[1]):
                uv[i, 2*j] = u[i, j]
                uv[i, 2*j+1] = v[i, j]
        return uv
    else:
        print("size of Channel U is different with Channel V")


def rgb2nv12(image):
    if image.ndim == 3:
        b = image[:, :, 0]
        g = image[:, :, 1]
        r = image[:, :, 2]
        y = (0.299*r+0.587*g+0.114*b)
        u = (-0.169*r-0.331*g+0.5*b+128)[::2, ::2]
        v = (0.5*r-0.419*g-0.081*b+128)[::2, ::2]
        uv = mergeUV(u, v)
        yuv = np.vstack((y, uv))
        return yuv.astype(np.uint8)
    else:
        print("image is not BGR format")

file = sys.argv[1]
img = cv2.imread(file)
yuv = rgb2nv12(img)
yuv.tofile(file + ".nv12")
print("done")

