#ifndef UDPRECEIVER_H
#define UDPRECEIVER_H

#include <QObject>
#include <QUdpSocket>

class UdpReceiver : public QObject
{
    Q_OBJECT
public:
    explicit UdpReceiver(QObject *parent = nullptr);
    void startListening(int port); // Dinlemeyi başlatan fonksiyon

signals:
    void dataReceived(QString message); // Arayüze "veri geldi!" diye haber veren sinyal

private slots:
    void processPendingDatagrams(); // Veri geldiğinde çalışan slot

private:
    QUdpSocket *socket;

};


#endif // UDPRECEIVER_H
