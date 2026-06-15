from rknn.api import RKNN

# 1. 初始化 RKNN 转换引擎 (指定针对 RK3588/Rock 5T 平台)
rknn = RKNN()

# 2. 配置参数（锁定我们代码里的 NHWC 或者是标准的 RGB 物理格式）
rknn.config(
    mean_values=[[0, 0, 0]], 
    std_values=[[255, 255, 255]], # 硬件级归一化
    target_platform='rk3588'       # 🚀 物理死锁核心
)

# 3. 载入你的 YOLO ONNX 模型
print("加载 ONNX 模型...")
ret = rknn.load_onnx(model='yolov8s.onnx') # 替换成你真实的onnx路径

# 4. 编译模型（构建 NPU 硬件能识别的动态图）
print("正在编译 NPU 算力硬核...")
ret = rknn.build(do_quantization=False) # 先选 False 跑通，后面可以搞 INT8 量化加速

# 5. 导出真正能用的真子弹！
print("正在导出真子弹...")
ret = rknn.export_rknn('models/face_detector.rknn')

rknn.release()
print("🎉 转换大捷！去 models 目录下拿真子弹！")