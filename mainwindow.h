#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "udpreceiver.h"
#include "qcustomplot.h"

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

    double key; // Zaman sayacı
    double filteredSpeed; // Önceki filtrelenmiş değeri tutmak için
    const double alpha = 0.1; // Filtre sertliği (%10 yeni veri, %90 eski veri)

};
#endif // MAINWINDOW_H
