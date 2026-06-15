import sys

# 🚀 物理合闸：直接调用被我们唤醒的板载原厂 rknnlite 运行时
try:
    from rknnlite.api import RKNNLite
except ImportError:
    print("❌ 依然找不到 rknnlite，请确认之前的物理验证命令是否正确！")
    sys.exit(-1)

# 1. 初始化板子本地轻量级 NPU 编译器
rknn = RKNNLite()

print("⚙️  正在锁定 Radxa 5T (RK3588) 硬件级配置...")
rknn.config(
    mean_values=[[0, 0, 0]],
    std_values=[[255, 255, 255]],
    target_platform='rk3588'
)

# 2. 吃进去刚传过来的 ONNX 原料
onnx_path = 'yolov5nu.onnx'
print(f"📦 正在载入算法原料: {onnx_path} ...")
ret = rknn.load_onnx(model=onnx_path)
if ret != 0:
    print("❌ 载入 ONNX 失败！")
    sys.exit(ret)

# 3. 驱动 NPU 芯片就地进行硬件级算子融合
print("🧠 板载 NPU 正在疯狂运转，进行硬件级拓扑融合...")
ret = rknn.build(do_quantization=False)
if ret != 0:
    print("❌ 物理构建失败！")
    sys.exit(ret)

# 4. 吐出 C++ 程序相认的 face_detector.rknn
output_path = 'models/face_detector.rknn'
print(f"💾 正在将生成的物理子弹强行拍进枪膛: {output_path} ...")
ret = rknn.export_rknn(output_path)

rknn.release()
print("\n🎉 [全量大捷] 杰哥！真子弹已在板子本地全部熔炼成功！")
