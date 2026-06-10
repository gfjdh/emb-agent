# NCNN 推理性能优化指南（飞腾 ARMv8）

## 飞腾 CPU 特性

- 架构：ARMv8 (aarch64)
- 核心数：4 核
- SIMD 指令集：NEON（支持 fp16、asimd）
- 加密扩展：aes、pmull、sha1、sha2、crc32、sha3、sha512
- 不支持：Vulkan（无独立 GPU 推理能力）

## 优化策略

### 1. 使用 INT8 量化模型（推荐）

板子上已提供两个模型版本：
- FP16 模型：`s.param` + `s.bin`（3.3MB）
- INT8 模型：`i8s.param` + `i8s.bin`（1.7MB）

INT8 量化在 ARM NEON 上推理速度比 FP16 快 30~40%，且模型体积减半。
在 `v5lite-s.cpp` 中将 `USE_INT8` 设为 `1`，然后重新编译即可切换。

### 2. NCNN 线程数调优

飞腾板为 4 核处理器，NCNN 默认使用全部核心。
建议设置 `net.opt.num_threads = 2` 仅使用大核，可减少大小核线程切换开销，提升约 15~20% 推理速度。

### 3. 复用 extractor

NCNN 推理时应复用 `ncnn::Extractor` 对象，避免每张图重复分配中间张量缓冲区。

### 4. 编译优化参数

编译时应使用：
```
-O2 -mcpu=native -mtune=native -fopenmp
```
启用 NEON 自动向量化和 OpenMP 多线程。

### 5. 进程优先级

推理进程设置较高调度优先级可减少被系统打断：
```bash
nice -n -10 ./build/ncnnv5lite images/horse.jpg
```

## 预期性能

- v5lite-s FP16 单图推理：约 340ms（实测）
- v5lite-s INT8 单图推理：约 220ms（预计）
- 温度：正常推理负载下 40~45°C

## 性能采集命令

```bash
# CPU
top -bn1 | head -5
# 内存
free -h
# 温度
cat /sys/class/thermal/thermal_zone*/temp
# 负载
uptime
```

## 注意事项

- 飞腾板无 CUDA/GPU 推理能力，不要建议 TensorRT 或 CUDA 相关方案
- PyTorch 在该板子上不兼容（非法指令），不要建议 `pip install torch`
- 板子用户名密码为 `user`/`user`，SSH 端口 22
- 板子默认 IP 为 `172.24.40.225`
