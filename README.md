# Panon - Deepin 25 Dock Plugin

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

## 构建

```bash
# 安装依赖
sudo apt install dde-dock-dev qt6-base-dev libpulse-dev cmake pkg-config

# 构建
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)

# 安装
sudo make install
pkill dde-dock
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
