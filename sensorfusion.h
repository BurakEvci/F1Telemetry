#ifndef SENSORFUSION_H
#define SENSORFUSION_H

// 1. SOYUT SINIF (INTERFACE)
// Tüm filtreler bu kalıba uymak zorundadır.
class IFusionStrategy
{
public:
    virtual ~IFusionStrategy() {}

    // Her filtrenin bir "Hesapla" fonksiyonu olmalı
    // input: Tahmin edici (İvme), measurement: Düzeltici (GPS)
    virtual double update(double measurement, double input, double dt) = 0;

    // Filtre ayarını değiştirmek için
    virtual void setGain(double val) = 0;
};

// 2. SOMUT SINIF (COMPLEMENTARY FILTER)
class ComplementaryFilter : public IFusionStrategy {
private:
    double currentEstimate = 0;
    double alpha = 0.98; // Varsayılan güven katsayısı

public:
    void setGain(double val) override {
        alpha = val;
    }

    double update(double measurement, double input, double dt) override {
        // Formül: %98 * (Eski + İvme*dt) + %2 * (GPS)

        // 1. Tahmin (Prediction) - İvme İntegrali
        double prediction = currentEstimate + (input * dt);

        // 2. Düzeltme (Correction) - GPS Verisi
        // Not: Gerçek hayatta burada "GPS geldi mi?" kontrolü yapılır.
        double correction = measurement;

        // 3. Füzyon
        currentEstimate = (alpha * prediction) + ((1.0 - alpha) * correction);

        return currentEstimate;
    }
};

// İLERİDE EKLENECEK OLAN (Örnek):
// class KalmanFilter : public IFusionStrategy { ... }

#endif // SENSORFUSION_H
