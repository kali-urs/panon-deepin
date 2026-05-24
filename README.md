# Panon - Deepin 25 Dock Plugin

[![Build](https://github.com/kali-urs/panon-deepin/actions/workflows/build.yml/badge.svg)](https://github.com/kali-urs/panon-deepin/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/kali-urs/panon-deepin)](https://github.com/kali-urs/panon-deepin/releases)

音频频谱可视化 dock 插件，在 Deepin 25 DDE 7.0 任务栏上实时显示系统音频频谱。

参考自 [rbn42/panon](https://github.com/rbn42/panon) —— 一个 KDE Plasma 音频可视化 Widget。

## 效果

支持 6 种视觉效果，右键菜单切换：

| 效果 | 说明 |
|------|------|
| **Bars** | 柱状频谱图 + 白色峰值保持线 |
| **Wave** | 波形线 |
| **Solid** | 闭合渐变填充区域 |
| **Beam** | 细光束轨迹 + 半透明填充 |
| **Hill** | 高斯平滑"山丘"曲线 |
| **Spectrogram** | 滚动频谱图 |

## 下载

从 [Releases](https://github.com/kali-urs/panon-deepin/releases) 页面下载最新版本。

### deb 安装

```bash
sudo apt install ./dde-dock-panon_*.deb
pkill dde-dock
```

### 手动安装

```bash
sudo cp libpanon.so /usr/lib/dde-dock/plugins/
pkill dde-dock
```

## 从源码构建

```bash
# 安装依赖
sudo apt install dde-dock-dev qt6-base-dev libpulse-dev cmake pkg-config

# 构建
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 安装
sudo make install
pkill dde-dock
```

## CI 在线构建

每次 push 到 `main` 或 `v*` tag 时，GitHub Actions 自动在 Deepin 25 Docker 容器中编译并生成 `.so` 和 `.deb` 产物。

tag 推送格式:

```bash
git tag v1.0.0
git push origin v1.0.0
```

## 架构

```
AudioSource (PulseAudio) → FFTProcessor → SpectrumWidget → VisualEffect[*]
                                         → WaveEffect, BarEffect, ...
```

- `AudioSource`: PulseAudio `pa_simple` 捕获系统音频，输出归一化样本
- `FFTProcessor`: 汉宁窗 + Cooley-Tukey FFT
- `SpectrumWidget`: 持有当前 `VisualEffect`，委托渲染
- `VisualEffect`: 抽象基类，子类实现不同视觉效果

## 参考

- 原始项目: [rbn42/panon](https://github.com/rbn42/panon) (GPL-3.0) — KDE Plasma 音频可视化，提供 22 种 GLSL shader 特效和 Python 音频后端
- Deepin Dock Plugin API: [linuxdeepin/dde-dock](https://github.com/linuxdeepin/dde-dock)

## 许可

GPL-3.0
