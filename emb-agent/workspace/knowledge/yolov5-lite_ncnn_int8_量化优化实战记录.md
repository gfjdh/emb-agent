# YOLOv5-Lite NCNN INT8 量化优化实战

## 环境
- 飞腾派 (Phytium-Pi), Ubuntu 20.04, 4核 ARM CPU
- ncnn 推理框架 + OpenCV
- YOLOv5-Lite 模型

## 优化步骤

### 1. 修改源码宏定义
在 `v5lite-s.cpp` 中将 `#define USE_INT8 0` 改为 `#define USE_INT8 1`

### 2. 重新编译
```bash
cd /home/user/Downloads/YOLOv5-Lite/cpp_demo/ncnn/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2 -mcpu=native -mtune=native -fopenmp"
make -j4
```

### 3. 运行测试
```bash
cd /home/user/Downloads/YOLOv5-Lite/cpp_demo/ncnn
./build/ncnnv5lite images/horse.jpg
```

## 性能对比

| 指标 | FP16 | INT8 | 提升 |
|------|------|------|------|
| 推理时间 | 112.54ms | 87.34ms | ↓22.4% |
| FPS | 8.88 | 11.45 | ↑28.9% |
| 模型大小 | 3.22MB | 1.72MB | ↓46.6% |
| CPU user time | 0.308s | 0.256s | ↓16.9% |

## 注意事项
- INT8 量化会损失部分检测精度（高置信度目标仍可检出，低置信度目标可能被过滤）
- 模型文件位置：`/home/user/Downloads/YOLOv5-Lite/cpp_demo/ncnn/model_zoo/`
- INT8 模型文件：`i8s.param` + `i8s.bin`
- FP16 模型文件：`s.param` + `s.bin`
- 编译优化参数 `-O2 -mcpu=native -mtune=native -fopenmp` 可进一步提升性能
