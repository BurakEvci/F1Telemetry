#include "telemetryengine.h"

TelemetryEngine::TelemetryEngine() : alpha(0.1)
{
    // Başlangıç değerleri 0

    // --- KRİTİK EKSİK 1: FİLTREYİ BAŞLATMA ---
    // Varsayılan olarak Complementary Filter takıyoruz.
    // İlerde burayı "new KalmanFilter()" yapabilirsin.
    fusionFilter = new ComplementaryFilter();
}

TelemetryEngine::~TelemetryEngine()
{
    // --- KRİTİK EKSİK 2: TEMİZLİK ---
    // Pointer sildiğimiz için mutlaka delete yapmalıyız.
    if (fusionFilter) {
        delete fusionFilter;
        fusionFilter = nullptr;
    }
}

void TelemetryEngine::setAlpha(double val)
{
    // Hem motorun kendi LPF filtresini güncelle
    alpha = val;

    // --- KRİTİK EKSİK 3: AYARI FİLTREYE İLET ---
    // Slider değerini füzyon algoritmasına da gönder
    if (fusionFilter) {
        fusionFilter->setGain(val);
    }
}

VehicleState TelemetryEngine::getState() const
{
    return currentState;
}


void TelemetryEngine::processData(QString message, double dt)
{
    // --- 1. PARSING (Ayrıştırma) ---
    QStringList parts = message.split(",");
    bool hasAccel = false;

    for (const QString &part : parts) {
        if (part.startsWith("SPEED:")) {
            double raw = part.section(':', 1, 1).toDouble();

            currentState.rawSpeed = raw;
            // Basit Low Pass Filter (Yeşil çizgi için)
            currentState.filteredSpeed = (raw * alpha) + (currentState.filteredSpeed * (1.0 - alpha));
        }
        else if (part.startsWith("ACCEL:")) {
            double raw = part.section(':', 1, 1).toDouble();

            currentState.rawAccel = raw;
            // Basit Low Pass Filter (İvme analizi için)
            currentState.filteredAccel = (raw * alpha) + (currentState.filteredAccel * (1.0 - alpha));
            hasAccel = true;
        }
    }

    // --- 2. FİZİK MOTORU VE FÜZYON ---
    if (hasAccel) {
        // A. Sadece İntegral (Drift'i görmek için - Kırmızı Çizgi)
        currentState.calcSpeed = currentState.calcSpeed + (currentState.rawAccel * dt);

        // --- KRİTİK EKSİK 4: SENSÖR FÜZYONU (Sarı Çizgi) ---
        // Strateji desenini kullanarak hesaplama yapıyoruz.
        if (fusionFilter) {
            currentState.fusedSpeed = fusionFilter->update(
                currentState.rawSpeed, // Measurement (GPS - Düzeltici)
                currentState.rawAccel, // Input (İvme - Tahmin Edici)
                dt                     // Zaman farkı
                );
        }
    }
}
