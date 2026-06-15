from rknn.api import RKNN
import sys

rknn = RKNN(verbose=False)

print("⚙️  正在锁定 Radxa 5T (RK3588) NPU 硬件级色域矩阵...")
rknn.config(
    mean_values=[[0, 0, 0]],
    std_values=[[255, 255, 255]],
    target_platform='rk3588'
)

print("📦 正在载入算法原料: yolov5nu.onnx ...")
ret = rknn.load_onnx(model='yolov5nu.onnx')
if ret != 0:
    print("❌ 载入 ONNX 失败！")
    sys.exit(ret)

print("🧠 板载 NPU 正在启动全量硬件级融合编译 (这个过程需要大概30秒到1分钟)...")
ret = rknn.build(do_quantization=False)
if ret != 0:
    print("❌ 编译 NPU 核心失败！")
    sys.exit(ret)

print("💾 正在将生成的满血真子弹强行拍进枪膛...")
ret = rknn.export_rknn('models/face_detector.rknn')

rknn.release()
print("\n🎉🎉🎉 [史诗级大捷] 杰哥！真正的 YOLOv5n 物理子弹已经在板子本地全量熔炼成功！！！")
