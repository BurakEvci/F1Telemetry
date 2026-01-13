#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCheckBox>
#include "udpreceiver.h"
#include "qcustomplot.h"
#include "telemetrytypes.h"
#include "telemetryengine.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Backend'den gelen sinyali yakalayacak slot
    void updateTelemetry(QString message);

private:
    Ui::MainWindow *ui;

    UdpReceiver *telemetryLink; // Bizim alıcı nesnemiz
    QCustomPlot *customPlot;
    TelemetryEngine *engine;

    QCheckBox *chkLive;

    double key; // Zaman sayacı
    double alpha; // Filtre sertliği (%10 yeni veri, %90 eski veri)


    // Eski 'filteredSpeed' değişkenini sildik.
    // Artık tüm veriler (Speed, RPM, Accel) bu haritada tutulacak.
    // Erişim: channels["SPEED"]->filteredValue
    QMap<QString, TelemetryChannel*> channels;


    // Yeni kanal kurulumunu otomatikleştiren yardımcı fonksiyon
    // Örn: setupChannel("SPEED", Qt::red);
    void setupChannel(QString name, QColor color);


    // Helper Functions (Temizlikçiler)
    void setupUI();       // Slider, Checkbox vb.
    void setupCharts();   // QCustomPlot ayarları
    void setupNetwork();  // UDP başlatma
    void setupChannels(); // speed, accel, fusion kanallarını kurma

};
#endif // MAINWINDOW_H
