import cv2
import numpy as np
from rknn.api import RKNN

# 配置
MODEL_PATH = 'models/face_detector.rknn'
IMAGE_PATH = 'build/faces/12345_1781083162030.jpg'

# 初始化RKNN
rknn = RKNN()
print('--> Loading RKNN model')
ret = rknn.load_rknn(MODEL_PATH)
if ret != 0:
    print('Load RKNN model failed')
    exit(ret)
print('done')

# 初始化运行时
print('--> Init runtime')
ret = rknn.init_runtime(target='rk3588')
if ret != 0:
    print('Init runtime failed')
    exit(ret)
print('done')

# 准备输入图像
img = cv2.imread(IMAGE_PATH)
if img is None:
    print(f"Failed to load image: {IMAGE_PATH}")
    exit(1)

# 模型输入尺寸 640x640
img_resized = cv2.resize(img, (640, 640))
img_input = np.expand_dims(img_resized, axis=0)

# 推理
print('--> Running inference')
outputs = rknn.inference(inputs=[img_input])
print('done')

# 分析输出
out = outputs[0]
print(f"Output shape: {out.shape}")

# 转换为 (84, 8400)
data = out.reshape(84, 8400)

# 打印第一个完整框的所有84个值
print("\n=== 第一个框 (index 0) 的全部84个值 ===")
box0 = data[:, 0]
for i in range(84):
    print(f"  [{i:2d}] = {box0[i]}")

# 找出可能的最小值和最大值
print(f"\n第一个框的值范围: min={box0.min():.2f}, max={box0.max():.2f}")

# 打印前几个框的坐标候选
print("\n=== 前5个框的坐标候选 (前4个值) ===")
for i in range(5):
    print(f"Box {i}: x={data[0,i]:.2f}, y={data[1,i]:.2f}, w={data[2,i]:.2f}, h={data[3,i]:.2f}")

rknn.release()
