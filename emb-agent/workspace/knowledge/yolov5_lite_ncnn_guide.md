# YOLOv5-Lite NCNN 推理指南（飞腾 ARMv8）

## 项目位置

YOLOv5-Lite 源码位于：`/home/user/Downloads/YOLOv5-Lite/`

NCNN 库位于：`/home/user/Downloads/Yolo-FastestV2/sample/ncnn/ncnn/`

## 预编译 NCNN 二进制（已可用，无需重新编译）

C++ NCNN demo 已编译完成：
- 可执行文件：`/home/user/Downloads/YOLOv5-Lite/cpp_demo/ncnn/build/ncnnv5lite`
- 源码文件：`/home/user/Downloads/YOLOv5-Lite/cpp_demo/ncnn/v5lite-s.cpp`

## 模型文件

模型位于 `/home/user/Downloads/YOLOv5-Lite/cpp_demo/ncnn/model_zoo/`：
- FP16 模型：`s.param` (17KB) + `s.bin` (3.3MB)
- INT8 量化模型：`i8s.param` (18KB) + `i8s.bin` (1.7MB)

## 运行方式

```bash
cd /home/user/Downloads/YOLOv5-Lite/cpp_demo/ncnn
./build/ncnnv5lite images/horse.jpg
```

输出结果保存在 `result.jpg`，终端会打印检测框坐标、类别、置信度和推理耗时。

## 测试图片

测试图片位于：`/home/user/Downloads/YOLOv5-Lite/cpp_demo/ncnn/images/horse.jpg`

## NCNN 库路径

- 头文件：`/home/user/Downloads/Yolo-FastestV2/sample/ncnn/ncnn/build/install/include/ncnn/`
- 库文件：`/home/user/Downloads/Yolo-FastestV2/sample/ncnn/ncnn/build/install/lib/libncnn.so`

## 重要：不要用 Python detect.py

`detect.py` 需要 PyTorch，而 PyTorch 在飞腾 ARM 上会报 `非法指令` 错误。必须使用 C++ NCNN 版本。

## 自定义图片推理

```bash
./build/ncnnv5lite /path/to/your_image.jpg
```
