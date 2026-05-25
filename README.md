# Panon — Deepin 25 Dock 音频频谱

[![Build](https://github.com/kali-urs/panon-deepin/actions/workflows/build.yml/badge.svg)](https://github.com/kali-urs/panon-deepin/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/kali-urs/panon-deepin)](https://github.com/kali-urs/panon-deepin/releases)

Deepin 25 (DDE 7.0) 任务栏音频频谱可视化插件，6 种视觉效果，支持双声道、幻彩、宽度调节。

## 效果

柱状、波形、实心、光束、山丘、频谱图

## 安装

从 [Releases](https://github.com/kali-urs/panon-deepin/releases) 下载 `.deb`：

```bash
sudo apt install ./dde-dock-panon_*.deb
pkill dde-dock
```

### 从源码构建

```bash
sudo apt install dde-dock-dev qt6-base-dev libpulse-dev cmake pkg-config
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
pkill dde-dock
```

## 许可

GPL-3.0
