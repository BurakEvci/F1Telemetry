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


    setupChannel("SPEED", Qt::green);

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

    // --- 1. PARSING (Veriyi Ayıkla) ---
    // Gelen veri formatı: "SPEED:155.40"
    double rawSpeed = 0;

    // Güvenlik kontrolü: Mesaj boş mu veya beklenen formatta mı?
    if (message.startsWith("SPEED:")) {
        // "SPEED:" kısmından sonrasını al ve sayıya çevir
        rawSpeed = message.section(':', 1, 1).toDouble();
    } else {
        // Eğer SPEED verisi yoksa fonksiyondan çık (Hata almamak için)
        return;
    }

    // --- 2. VERİ YÖNETİMİ (MAP KULLANIMI) ---
    // "SPEED" kanalını haritadan çek
    if (!channels.contains("SPEED")) return; // Güvenlik kontrolü
    TelemetryChannel *speedCh = channels["SPEED"];



    // --- 3. SİNYAL İŞLEME (Low Pass Filter) ---
    //speedCh nesnesinin içindeki değeri güncelliyoruz
    speedCh->filteredValue = (rawSpeed * alpha) + (speedCh->filteredValue * (1.0 - alpha));
    speedCh->rawValue = rawSpeed; // Son ham veriyi de saklayalım


    // --- 4. GÖRSELLEŞTİRME (Plotting) ---
    // Artık graph(0) veya graph(1) yok. Kanalın kendi grafiğine ekliyoruz.
    speedCh->rawGraph->addData(key, speedCh->rawValue);
    speedCh->filteredGraph->addData(key, speedCh->filteredValue);


    // --- 5. EKSEN GÜNCELLEME ---
    // X eksenini kaydır (Zaman aktıkça grafik sağa kaysın)
    // Son 8 saniyeyi gösteriyoruz
    customPlot->xAxis->setRange(key, 8, Qt::AlignRight);
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

    // 3. Filtreli Veri Grafiğini (Kalın Çizgi) oluştur
    // Rengi biraz koyulaştırarak fark yaratalım
    newChannel->filteredGraph = customPlot->addGraph();
    newChannel->filteredGraph->setPen(QPen(color.darker(150), 3)); // Kalın ve koyu
    newChannel->filteredGraph->setName(name + " (Filtered)");

    // 4. Haritaya (Map) kaydet
    // Artık bu kanala isminden ulaşabileceğiz: channels["SPEED"]
    channels.insert(name, newChannel);
}




