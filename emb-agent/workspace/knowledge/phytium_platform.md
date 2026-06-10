# Phytium Platform Development

## Processor Families
- Phytium D2000 - Desktop processor
- Phytium S2500 - Server processor
- Phytium FT-2000 - Embedded processor

## Boot Process
1. BootROM (internal)
2. U-Boot (or other bootloader)
3. Linux Kernel
4. Root Filesystem

## Buildroot Integration
```bash
# Configure for Phytium
make CROSS_COMPILE=aarch64-linux-gnu- phytium_defconfig
# Customize
make menuconfig
# Build
make
```

## Cross-Compilation
```bash
export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64
# Build kernel
make defconfig
make -j$(nproc)
```

## Development Board Setup
1. Set boot switches to SD/TFTP mode
2. Connect serial console (115200 8N1)
3. Configure network for TFTP boot
