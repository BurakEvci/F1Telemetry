#ifndef TELEMETRYTYPES_H
#define TELEMETRYTYPES_H

#include <QString>
#include <qcustomplot.h> // Grafik pointerlarını tutacağız

// Her bir veri kanalı (Speed, RPM, G-Force) bu yapıda tutulacak
struct TelemetryChannel {
    QString name;           // Kanal Adı (Örn: "SPEED")
    double rawValue;        // Son gelen ham veri
    double filteredValue;   // Filtrelenmiş veri

    QCPGraph *rawGraph;     // Kırmızı çizgiye işaretçi
    QCPGraph *filteredGraph;// Yeşil çizgiye işaretçi

    // Constructor (Başlangıç değerleri)
    TelemetryChannel() : rawValue(0), filteredValue(0), rawGraph(nullptr), filteredGraph(nullptr) {}
};

#endif // TELEMETRYTYPES_H
