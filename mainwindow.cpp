#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , key(0)
    , alpha(0.98)

{
    ui->setupUi(this);

    // Motoru Başlat
    engine = new TelemetryEngine();

    // Sadece fonksiyonları çağırıyoruz. Ne kadar temiz değil mi?
    setupUI();
    setupCharts();
    setupChannels(); // setupChannel çağrılarını bunun içine koyacağız
    setupNetwork();
}

MainWindow::~MainWindow()
{
    delete ui;
    // engine ve telemetryLink ebeveyn (parent) aldığı için veya
    // smart pointer kullanılmadığı için burada manuel silinebilir ama zorunlu değil (Qt temizler).
}


// --- ALT FONKSİYONLAR ---

void MainWindow::setupUI()
{
    // Log Ekranı
    ui->logViewer->setStyleSheet("QTextEdit { background-color: black; color: #00FF00; font-family: Consolas; border: none; }");
    ui->logViewer->append("Sistem Başlatılıyor...");

    // CheckBox
    chkLive = new QCheckBox("Canlı Akış (Zoom için kapat)", this);
    chkLive->setChecked(true);
    chkLive->setStyleSheet("color: white; font-weight: bold; font-size: 10pt; margin-left: 10px;");


    if (ui->centralwidget->layout()) {
        // En garanti yöntem: Slider'ın bulunduğu layout'u bulup oraya eklemek
        QLayout *controlLayout = ui->alphaSlider->parentWidget()->layout();
        if (controlLayout) {
            controlLayout->addWidget(chkLive);
        } else {
            // Hiçbir yer bulamazsa pencerenin en altına ekle
            ui->centralwidget->layout()->addWidget(chkLive);
        }
    }



    // Slider
    ui->alphaSlider->setRange(50, 100); // 0.50 - 1.00 arası mantıklı
    ui->alphaSlider->setValue(98);      // 0.98

    connect(ui->alphaSlider, &QSlider::valueChanged, this, [=](int value){
        alpha = value / 100.0;
        ui->lblAlphaValue->setText(QString("Gain: %1").arg(alpha));
    });
}


void MainWindow::setupCharts()
{
    customPlot = new QCustomPlot();

    // Eğer plotContainer'ın layout'u yoksa oluştur
    if (!ui->plotContainer->layout()) {
        QVBoxLayout *layout = new QVBoxLayout(ui->plotContainer);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(customPlot);
    } else {
        ui->plotContainer->layout()->addWidget(customPlot);
    }

    customPlot->xAxis->setLabel("Zaman (sn)");
    customPlot->yAxis->setLabel("Hız (km/h)");
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);


    // --- YENİ: LEGEND (AÇIKLAMA KUTUSU) ---
    customPlot->legend->setVisible(true);

    // Yazı tipi ayarı
    QFont legendFont = font();
    legendFont.setPointSize(9);
    customPlot->legend->setFont(legendFont);

    // Konumu (Sağ Üst Köşe)
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignLeft);

    // Arka planı yarı şeffaf beyaz yapalım ki grafik görünsün
    customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));

}


void MainWindow::setupChannels()
{
    setupChannel("SPEED (GPS)", Qt::green);       // Referans
    setupChannel("CALC_SPEED (ACC,IMU)", Qt::red);    // Drift
    setupChannel("FUSION", Qt::blue);     // Çözüm (Süper İnsan)

    // İvme (Mavi) opsiyonel
    // setupChannel("ACCEL", Qt::blue);
}


void MainWindow::setupNetwork()
{
    telemetryLink = new UdpReceiver(this);
    connect(telemetryLink, &UdpReceiver::dataReceived, this, &MainWindow::updateTelemetry);
    telemetryLink->startListening(5555);
    ui->logViewer->append("UDP Dinleniyor: Port 5555");
}


// Grafik oluşturma mantığı
void MainWindow::setupChannel(QString name, QColor color)
{
    TelemetryChannel *newChannel = new TelemetryChannel();
    newChannel->name = name;

    // Ham Veri (Raw) - Gizli
    newChannel->rawGraph = customPlot->addGraph();
    newChannel->rawGraph->setPen(QPen(color, 1));
    newChannel->rawGraph->setName(name + " (Raw)");
    newChannel->rawGraph->setVisible(false);

    newChannel->rawGraph->removeFromLegend();

    // Filtreli Veri (Filtered) - Görünür
    newChannel->filteredGraph = customPlot->addGraph();
    newChannel->filteredGraph->setPen(QPen(color, 3));
    newChannel->filteredGraph->setName(name);

    channels.insert(name, newChannel);
}

// Veri güncelleme mantığı
void MainWindow::updateTelemetry(QString message)
{
    // 1. MOTORA VERİ GÖNDER
    engine->setAlpha(alpha);
    engine->processData(message, 0.05); // dt = 0.05

    // 2. SONUCU AL
    VehicleState state = engine->getState();

    // 3. GRAFİKLERİ GÜNCELLE

    // Yeşil (GPS)
    if (channels.contains("SPEED")) {
        channels["SPEED"]->rawGraph->addData(key, state.rawSpeed);
        channels["SPEED"]->filteredGraph->addData(key, state.filteredSpeed);
    }

    // Kırmızı (Drift)
    if (channels.contains("CALC_SPEED")) {
        channels["CALC_SPEED"]->filteredGraph->addData(key, state.calcSpeed);
    }

    // --- SARI (FÜZYON) ---
    // İşte aradığımız çözüm burası!
    if (channels.contains("FUSION")) {
        channels["FUSION"]->filteredGraph->addData(key, state.fusedSpeed);
    }

    // 4. EKSEN VE ÇİZİM
    if (chkLive->isChecked()) {
        customPlot->xAxis->setRange(key, 10, Qt::AlignRight);
        customPlot->yAxis->setRange(-50, 350);
    }

    customPlot->replot(QCustomPlot::rpQueuedReplot);
    key += 0.05;
}





