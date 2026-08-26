#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHostAddress>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QUdpSocket>
#include <QVBoxLayout>

namespace {
constexpr quint16 kLedCommandPort = 5055;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , udpSocket(new QUdpSocket(this))
{
    ui->setupUi(this);

    auto *layout = new QVBoxLayout(ui->centralwidget);
    auto *title = new QLabel("STM32 LED Control (PB0)", this);
    ipSelector = new QComboBox(this);
    ipSelector->setEditable(true);
    ipSelector->setInsertPolicy(QComboBox::InsertAtBottom);
    ipSelector->addItem("10.252.62.51");
    ipSelector->addItem("10.252.62.52");
    ipSelector->addItem("10.252.62.53");
    ipSelector->addItem("10.252.62.54");
    ipSelector->setToolTip("Select an STM32 IP address or enter a different one.");

    auto *onButton = new QPushButton("Turn LED On", this);
    auto *offButton = new QPushButton("Turn LED Off", this);
    auto *toggleButton = new QPushButton("Toggle LED", this);
    auto *refreshButton = new QPushButton("Refresh Status", this);
    statusLabel = new QLabel("Select an STM32 IP address.", this);
    statusLabel->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(ipSelector);
    layout->addWidget(onButton);
    layout->addWidget(offButton);
    layout->addWidget(toggleButton);
    layout->addWidget(refreshButton);
    layout->addWidget(statusLabel);
    layout->addStretch();

    setWindowTitle("STM32 LED Control");
    resize(360, 280);

    connect(onButton, &QPushButton::clicked, this, [this] { sendCommand("PB0 ON"); });
    connect(offButton, &QPushButton::clicked, this, [this] { sendCommand("PB0 OFF"); });
    connect(toggleButton, &QPushButton::clicked, this, [this] { sendCommand("PB0 TOGGLE"); });
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::requestStatus);
    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readReply);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::sendCommand(const QString &command)
{
    const QHostAddress boardAddress(ipSelector->currentText().trimmed());
    if (boardAddress.isNull()) {
        statusLabel->setText("The STM32 IP address is invalid.");
        return;
    }

    udpSocket->writeDatagram(command.toUtf8(), boardAddress, kLedCommandPort);
    statusLabel->setText("Sending: " + command);
}

void MainWindow::requestStatus()
{
    sendCommand("PB0 STATUS");
}

void MainWindow::readReply()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray reply;
        reply.resize(static_cast<int>(udpSocket->pendingDatagramSize()));
        udpSocket->readDatagram(reply.data(), reply.size());

        if (reply == "OK")
            requestStatus();
        else if (reply == "PB0 ON")
            statusLabel->setText("LED status: ON");
        else if (reply == "PB0 OFF")
            statusLabel->setText("LED status: OFF");
        else
            statusLabel->setText("STM32 reply: " + QString::fromUtf8(reply));
    }
}
