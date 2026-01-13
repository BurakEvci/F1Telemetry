#ifndef TELEMETRYENGINE_H
#define TELEMETRYENGINE_H

#include <QString>
#include <QStringList>
#include "sensorfusion.h"

// Tüm fiziksel durumumuzu tutan paket
struct VehicleState {
    double rawSpeed = 0;       // GPS Hızı (Ham)
    double filteredSpeed = 0;  // GPS Hızı (Filtreli)

    double rawAccel = 0;       // İvme (Ham)
    double filteredAccel = 0;  // İvme (Filtreli)

    double calcSpeed = 0;      // İntegral ile hesaplanan Hız (Drift yapan)

    double fusedSpeed = 0;     // Sensör Füzyonu Sonucu (SARI ÇİZGİ)
};


class TelemetryEngine
{
public:
    TelemetryEngine();
    ~TelemetryEngine(); // Pointer sileceğimiz için lazım

    // Veriyi işle (Parsing + Fizik Hesapları)
    void processData(QString message, double dt);

    // Filtre katsayısını ayarla (Slider'dan gelecek)
    void setAlpha(double val);

    // Güncel durumu dışarı ver (Arayüz için)
    VehicleState getState() const;


private:
    VehicleState currentState; // Anlık durum
    double alpha;              // Filtre katsayısı

    // Motor artık spesifik bir filtreyi değil, soyut arayüzü tanıyor.
    // Böylece buraya Complementary de takabilirsin, Kalman da.
    IFusionStrategy *fusionFilter;

};

#endif // TELEMETRYENGINE_H
