# 技术数据总压缩包说明

## 一、内容清单

- source/
  - YOLOv5-Lite NCNN 推理工程源码（C++）
  - 模型文件与示例图片
- bin/
  - 可执行文件：ncnnv5lite
- tests/
  - 验证测试脚本：run_smoke_test.sh

## 二、软硬件工作平台

- 硬件平台：飞腾四核处理器（ARMv8，2x FTC664@1.8GHz + 2x FTC310@1.5GHz）
- 操作系统：Ubuntu (arm64)
- 编译工具：GCC 9.4.0, CMake 3.16
- 依赖库：OpenCV 4.2, NCNN (本地编译)

## 三、构建与运行步骤

### 1. 构建

```bash
cd source
mkdir -p build
cd build
cmake .. -Dncnn_DIR=/path/to/ncnn/build/install/lib/cmake/ncnn
make -j$(nproc)
```

说明：
- 需要先编译并安装 NCNN，且 `ncnn_DIR` 指向本地安装目录。

### 2. 运行

```bash
cd source
../bin/ncnnv5lite images/horse.jpg
```

运行后会在 `source/` 目录生成 `result.jpg`。

## 四、测试验证步骤

```bash
cd tests
./run_smoke_test.sh
```

脚本会调用可执行文件，并检查 `result.jpg` 是否生成。

## AI工具使用声明

- 使用的AI工具名称、版本：GPT-5.2-Codex（GitHub Copilot）
- 使用场景与用途：辅助撰写技术说明与步骤整理
- AI生成内容占比（文字/代码/设计）：文字约80%，代码约50%，设计约50%
