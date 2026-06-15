import numpy as np
import os
from rknnlite.api import RKNNLite

rknn = RKNNLite()
model_path = 'models/face_detector.rknn'

if not os.path.exists(model_path):
    print(f"❌ 未找到模型文件: {model_path}")
    exit()

print(f"📦 正在加载模型进行底层解剖: {model_path} ...")
if rknn.load_rknn(model_path) != 0:
    print("❌ 模型加载失败"); exit()
if rknn.init_runtime() != 0:
    print("❌ 运行时初始化失败"); exit()

# 🚀 修正：YOLO 静态模型严格要求 4 维输入 [Batch, Height, Width, Channel]
# 构造一张全白/全黑的 1x640x640x3 图像送进去
fake_input = np.ones((1, 640, 640, 3), dtype=np.uint8)

print("⚡ 正在注入测试流进行单帧推理...")
outputs = rknn.inference(inputs=[fake_input])

print("\n" + "="*20 + " 💥 揭开底层真相的账本 💥 " + "="*20)
print(f"该模型共有 [{len(outputs)}] 个输出 Tensor (输出端)")

for i, out in enumerate(outputs):
    print(f"\n👉 输出端 [{i}]:")
    print(f"   - 矩阵形状 (Shape):    {out.shape}")
    print(f"   - 数据类型 (DataType): {out.dtype}")
    
    # 展平数据查看前 15 个数，探查真实数值区间
    flat = out.flatten()
    print(f"   - 内存前 15 个原始数值样本:")
    print("     ", [round(float(x), 4) for x in flat[:15]])
print("="*60)

rknn.release()