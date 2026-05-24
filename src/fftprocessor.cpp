#include "fftprocessor.h"

FFTProcessor::FFTProcessor(QObject *parent)
    : QObject(parent)
{
}

void FFTProcessor::setFFTSize(int size)
{
    m_fftSize = size;
}

void FFTProcessor::process(const QVector<double> &samples, QVector<double> &magnitudes)
{
    int len = samples.size();
    if (len == 0) return;

    QVector<std::complex<double>> data(len);
    for (int i = 0; i < len; ++i) {
        data[i] = std::complex<double>(samples[i], 0.0);
    }

    computeFFT(data, false);

    int outSize = len / 2;
    magnitudes.resize(outSize);
    for (int i = 0; i < outSize; ++i) {
        magnitudes[i] = std::abs(data[i]) / len;
    }
}

void FFTProcessor::applyHanningWindow(QVector<double> &samples)
{
    int n = samples.size();
    for (int i = 0; i < n; ++i) {
        samples[i] *= 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (n - 1)));
    }
}

void FFTProcessor::computeFFT(QVector<std::complex<double>> &a, bool invert)
{
    int n = a.size();
    if (n == 0) return;

    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }

    for (int len = 2; len <= n; len <<= 1) {
        double ang = 2 * M_PI / len * (invert ? -1 : 1);
        std::complex<double> wlen(std::cos(ang), std::sin(ang));
        for (int i = 0; i < n; i += len) {
            std::complex<double> w(1);
            for (int j = 0; j < len / 2; ++j) {
                std::complex<double> u = a[i + j];
                std::complex<double> v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    if (invert) {
        for (auto &x : a)
            x /= n;
    }
}
