#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , key(0)
    , alpha(0.1)

{
    ui->setupUi(this);

    // --- 1. QCustomPlot KURULUMU ---
    customPlot = new QCustomPlot();

    // Grafiği, Designer'da açtığımız 'plotContainer' kutusunun içine yerleştiriyoruz
    QVBoxLayout *layout = new QVBoxLayout(ui->plotContainer);
    layout->setContentsMargins(0, 0, 0, 0); // Kenar boşluklarını sıfırla
    layout->addWidget(customPlot);


    // --- 2. CANLI TAKİP KUTUCUĞU (YENİ) ---
    // Slider'ın yanına bir "Canlı Takip" kutusu ekleyelim
    // (Slider'ın olduğu layout'a erişip ekliyoruz)
    // Eğer Designer'da slider için bir layout yapmadıysan kodla ekleyelim:

    chkLive = new QCheckBox("Canlı Akış (Zoom için kapat)", this);
    chkLive->setChecked(true); // Başlangıçta canlı aksın
    chkLive->setStyleSheet("color: white; font-weight: bold;");

    // Slider'ın olduğu layout'u bulup oraya ekleyelim (ui->verticalLayout veya benzeri)
    // Veya basitçe slider'ın üstüne/altına koyabiliriz.
    // Şimdilik plotContainer'ın olduğu ana layout'a ekleyelim:
    layout->addWidget(chkLive);







    // Eksen Etiketleri
    customPlot->xAxis->setLabel("Zaman (sn)");
    customPlot->yAxis->setLabel("Hız (km/h)");

    // Eksen Aralıkları (Başlangıç)
    customPlot->xAxis->setRange(0, 10);
    customPlot->yAxis->setRange(0, 350);

    // Mouse ile grafiği kaydırabilir ve zoom yapabilirsiniz
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);


    // Alıcıyı oluştur
    telemetryLink = new UdpReceiver(this);
    // Sinyal-Slot bağlantısını kur (Backend -> Frontend)
    connect(telemetryLink, &UdpReceiver::dataReceived, this, &MainWindow::updateTelemetry);
    // Dinlemeyi başlat
    telemetryLink->startListening(5555);


    setupChannel("SPEED", Qt::green);       // Referans (Gerçek) Hız
    setupChannel("ACCEL", Qt::blue);        // İvme Verisi
    setupChannel("CALC_SPEED", Qt::red);    // Bizim Hesapladığımız Hatalı Hız


    // Log penceresini terminal gibi yapalım
    ui->logViewer->setStyleSheet("QTextEdit { background-color: black; color: #00FF00; font-family: Consolas; font-size: 10pt; border: none; }");
    ui->logViewer->append("Sistem Hazır. Port 5555 dinleniyor...");
    ui->logViewer->append("Kanal Yapısı: SPEED kanalı oluşturuldu.");


    // --- SLIDER AYARLARI ---
    // Slider 0 ile 100 arasında değer üretsin (Biz bunu 100'e bölüp kullanacağız)
    ui->alphaSlider->setRange(0,100),
    ui->alphaSlider->setValue(10); // Başlangıçta 0.1 olması için 10 yapıyoruz


    connect(ui->alphaSlider, &QSlider::valueChanged, this, [=](int value){
        // Slider'dan gelen 0-100 değerini 0.0-1.0 arasına çevir
        alpha = value / 100.0;
        // Ekrana güncel değeri yaz
        ui->lblAlphaValue->setText(QString("Alpha: %1").arg(alpha));
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateTelemetry(QString message)
{

    // --- 1. PARSING (Çoklu Veri) ---
    // Gelen: "SPEED:50.5,ACCEL:1.2"
    QStringList parts = message.split(",");

    double rawSpeed = 0;
    double rawAccel = 0;
    bool hasSpeed = false;
    bool hasAccel = false;

    for (const QString &part : parts) {
        if (part.startsWith("SPEED:")) {
            rawSpeed = part.section(':', 1, 1).toDouble();
            hasSpeed = true;
        }
        else if (part.startsWith("ACCEL:")) {
            rawAccel = part.section(':', 1, 1).toDouble();
            hasAccel = true;
        }
    }


    // --- 2. SPEED KANALI (Referans) ---
    if (hasSpeed && channels.contains("SPEED")) {
        TelemetryChannel *ch = channels["SPEED"];
        // LPF Filtre
        ch->filteredValue = (rawSpeed * alpha) + (ch->filteredValue * (1.0 - alpha));
        ch->rawValue = rawSpeed;

        ch->rawGraph->addData(key, ch->rawValue);
        ch->filteredGraph->addData(key, ch->filteredValue);
    }

    // --- 3. ACCEL KANALI (Görselleştirmek için) ---
    if (hasAccel && channels.contains("ACCEL")) {
        TelemetryChannel *ch = channels["ACCEL"];
        ch->filteredValue = (rawAccel * alpha) + (ch->filteredValue * (1.0 - alpha));
        //ch->rawValue = rawAccel;

        // İvme grafiğini şimdilik çizmesek de olur, hız grafiğini karıştırmasın.
        // Ama veri elimizde olsun.
    }


    // --- 4. KRİTİK BÖLÜM: HIZ HESABI (İNTEGRAL) ---
    // V_yeni = V_eski + (İvme * dt)
    if (hasAccel && channels.contains("CALC_SPEED")) {
        TelemetryChannel *calcCh = channels["CALC_SPEED"];

        // Zaman adımı (Python'daki sleep süresiyle uyumlu olmalı)
        double dt = 0.05;

        // HESAPLAMA: İvme verisini (rawAccel) sürekli topluyoruz.
        // Python'da bilerek 2.0 birim hata koyduk. Bakalım ne olacak?
        double newVelocity = calcCh->filteredValue + (rawAccel * dt);

        calcCh->filteredValue = newVelocity;

        // Grafiğe ekle (Kırmızı Çizgi)
        calcCh->filteredGraph->addData(key, newVelocity);
    }

    // --- 5. EKSEN GÜNCELLEME ---
    // Son 10 saniyeyi gösteriyoruz
    // Sadece "Canlı Akış" kutusu işaretliyse grafiği zorla kaydır
    if (chkLive->isChecked()) {
        customPlot->xAxis->setRange(key, 10, Qt::AlignRight);
        customPlot->yAxis->setRange(-50, 350); // Y eksenini de sabitle
    }
    // Eğer işaretli DEĞİLSE, hiçbir şeye dokunma. Kullanıcı zoom yapsın.

    // Çizimi güncelle
    customPlot->replot(QCustomPlot::rpQueuedReplot);

    // Zaman sayacını artır (Simülasyon hızıyla uyumlu artış)
    key += 0.05;
}

void MainWindow::setupChannel(QString name, QColor color)
{
    // 1. Yeni bir kanal yapısı oluştur (Heap bellekte)
    TelemetryChannel *newChannel = new TelemetryChannel();
    newChannel->name = name;

    // 2. Ham veri Grafiğini (İnce Çizgi) oluştur
    newChannel->rawGraph = customPlot->addGraph();
    newChannel->rawGraph->setPen(QPen(color, 1)); // İnce ve seçilen renkte
    newChannel->rawGraph->setName(name + " (Raw)");
    newChannel->rawGraph->setVisible(false); // <--- GİZLEDİK (Kirlilik önleme)


    // 3. Filtreli Veri Grafiğini (Kalın Çizgi) oluştur
    // Rengi biraz koyulaştırarak fark yaratalım
    newChannel->filteredGraph = customPlot->addGraph();
    newChannel->filteredGraph->setPen(QPen(color, 3)); // Kalın ve koyu
    newChannel->filteredGraph->setName(name);

    // 4. Haritaya (Map) kaydet
    // Artık bu kanala isminden ulaşabileceğiz: channels["SPEED"]
    channels.insert(name, newChannel);
}




