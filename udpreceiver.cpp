#include "udpreceiver.h"

UdpReceiver::UdpReceiver(QObject *parent)
    : QObject{parent}
{
    socket = new QUdpSocket(this);
}

void UdpReceiver::startListening(int port)
{
    // 5555. Portu dinlemeye başla
    if(socket->bind(QHostAddress::Any, port)) {
        // Veri geldiği an (readyRead sinyali), bizim işleme fonksiyonumuzu çalıştır.
        connect(socket, &QUdpSocket::readyRead, this, &UdpReceiver::processPendingDatagrams);
    }
}

void UdpReceiver::processPendingDatagrams()
{
    // Paket olduğu sürece oku (While loop kritik, bazen aynı anda 10 paket gelir)
    while (socket->hasPendingDatagrams()) {
        QByteArray buffer;
        buffer.resize(socket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;

        // Paketi, göndereni ve portu oku
        socket->readDatagram(buffer.data(), buffer.size(), &sender, &senderPort);

        // Gelen veriyi String'e çevirip MainWindow'a fırlat
        QString dataStr = QString::fromUtf8(buffer);
        emit dataReceived(dataStr);
    }
}
