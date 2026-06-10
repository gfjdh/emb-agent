# NCNN 推理性能参考（飞腾 ARMv8）

## 飞腾 CPU 特性

- 架构：ARMv8 (aarch64)
- 核心数：4 核
- SIMD 指令集：NEON（支持 fp16、asimd）
- 加密扩展：aes、pmull、sha1、sha2、crc32、sha3、sha512
- 无 Vulkan 支持

## 可用的模型版本

板子上已提供两个模型版本，位于 `/home/user/Downloads/YOLOv5-Lite/cpp_demo/ncnn/model_zoo/`：
- FP16 模型：`s.param` + `s.bin`（3.3MB），`v5lite-s.cpp` 中 `USE_INT8` 设为 `0` 时使用
- INT8 模型：`i8s.param` + `i8s.bin`（1.7MB），`v5lite-s.cpp` 中 `USE_INT8` 设为 `1` 时使用

切换方式：修改 `v5lite-s.cpp` 中的 `#define USE_INT8` 宏，重新编译即可。

## NCNN 线程配置

NCNN 默认使用全部 CPU 核心。可通过 `net.opt.num_threads` 调整。

## 编译参数

编译时可使用 `-O2 -mcpu=native -mtune=native -fopenmp` 启用 NEON 自动向量化。

## 进程优先级

可通过 `nice -n -10` 运行推理程序提高调度优先级。

## 性能采集命令

```
# CPU
top -bn1 | head -5
# 内存
free -h
# 温度
cat /sys/class/thermal/thermal_zone*/temp
# 负载
uptime
```

## 平台限制

- 不支持 CUDA/GPU 推理
- PyTorch 在该板子上不兼容（非法指令）
- 板子 SSH 凭据：`user`/`user`，端口 22
- 板子默认 IP：`172.24.40.225`
