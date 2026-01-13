#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , key(0)
{
    ui->setupUi(this);

    // --- 1. QCustomPlot KURULUMU ---
    customPlot = new QCustomPlot(this);
    setCentralWidget(customPlot); // Pencerenin ortasına yerleştir

    // Grafik Ekleme (Graph 0) 1. Grafik: Ham Veri (Kırmızı ve İnce)
    customPlot->addGraph();
    customPlot->graph(0)->setPen(QPen(Qt::red)); // Çizgi rengi kırmızı
    customPlot->graph(0)->setName("Hız Verisi");

    // 2. Grafik: Filtrelenmiş Veri (Yeşil ve Kalın)
    customPlot->addGraph();
    customPlot->graph(1)->setPen(QPen(Qt::green, 3)); // Kalınlık 3
    customPlot->graph(1)->setName("Filtered (Smooth)");

    // filteredSpeed değişkenini başlat
    filteredSpeed = 0;


    // Eksen Etiketleri
    customPlot->xAxis->setLabel("Zaman (sn)");
    customPlot->yAxis->setLabel("Hız (km/h)");

    // Eksen Aralıkları (Başlangıç)
    customPlot->xAxis->setRange(0, 10);
    customPlot->yAxis->setRange(0, 350);

    // --- KRİTİK F1 ÖZELLİĞİ: ETKİLEŞİM ---
    // Mouse ile grafiği kaydırabilir ve zoom yapabilirsiniz
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);



    // Alıcıyı oluştur
    telemetryLink = new UdpReceiver(this);

    // Sinyal-Slot bağlantısını kur (Backend -> Frontend)
    connect(telemetryLink, &UdpReceiver::dataReceived, this, &MainWindow::updateTelemetry);

    // Dinlemeyi başlat
    telemetryLink->startListening(5555);

    ui->logViewer->append("Sistem Hazır. Port 5555 dinleniyor...");
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

    // --- 2. SİNYAL İŞLEME (Low Pass Filter) ---
    // Formül: Filtered = (Ham * alpha) + (Eski_Filtreli * (1 - alpha))
    // alpha ne kadar küçükse (örn: 0.05), grafik o kadar "yumuşak" olur ama gecikir.
    filteredSpeed = (rawSpeed * alpha) + (filteredSpeed * (1.0 - alpha));

    // --- 3. GÖRSELLEŞTİRME (Plotting) ---

    // Grafik 0 (Kırmızı): Ham Veri - Gürültüyü görmek için
    customPlot->graph(0)->addData(key, rawSpeed);

    // Grafik 1 (Yeşil): Filtrelenmiş Veri - Gerçek hareketi görmek için
    customPlot->graph(1)->addData(key, filteredSpeed);

    // --- 4. EKSEN VE GÜNCELLEME ---

    // X eksenini kaydır (Zaman aktıkça grafik sağa kaysın)
    // Son 8 saniyeyi gösteriyoruz
    customPlot->xAxis->setRange(key, 8, Qt::AlignRight);

    // Çizimi güncelle
    customPlot->replot(QCustomPlot::rpQueuedReplot);

    // Zaman sayacını artır (Simülasyon hızıyla uyumlu artış)
    key += 0.05;


}
