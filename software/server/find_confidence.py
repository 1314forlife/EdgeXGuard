import numpy as np
from rknn.api import RKNN
import cv2

rknn = RKNN()
rknn.load_rknn('models/face_detector.rknn')
rknn.init_runtime(target='rk3588')

img = cv2.imread('build/faces/12345_1781083162030.jpg')
img = cv2.resize(img, (640, 640))
img_input = np.expand_dims(img, axis=0)

outputs = rknn.inference(inputs=[img_input])
data = outputs[0].reshape(84, 8400)

# 找出哪个位置的数值在0-1之间
print("=== 查找置信度位置 ===")
for row in range(84):
    values = data[row, :]
    if values.min() >= 0 and values.max() <= 1:
        print(f"第{row}行: min={values.min():.4f}, max={values.max():.4f}, mean={values.mean():.4f}")

# 打印第一个框的所有值
print("\n=== 第一个框的所有84个值 ===")
box0 = data[:, 0]
for i in range(84):
    print(f"[{i}] = {box0[i]}")

rknn.release()
