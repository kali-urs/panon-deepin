#ifndef FFTPROCESSOR_H
#define FFTPROCESSOR_H

#include <QObject>
#include <QVector>
#include <complex>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class FFTProcessor : public QObject
{
    Q_OBJECT
public:
    explicit FFTProcessor(QObject *parent = nullptr);

    void setFFTSize(int size);
    int fftSize() const { return m_fftSize; }

    void process(const QVector<double> &samples, QVector<double> &magnitudes);

    static void applyHanningWindow(QVector<double> &samples);
    static void computeFFT(QVector<std::complex<double>> &data, bool invert);

private:
    int m_fftSize = 1024;
};

#endif
