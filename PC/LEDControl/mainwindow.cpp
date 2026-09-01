#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHostAddress>
#include <QComboBox>
#include <QDoubleSpinBox>
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
    rpmInput = new QDoubleSpinBox(this);
    rpmInput->setRange(-120.0, 120.0);
    rpmInput->setDecimals(1);
    rpmInput->setSuffix(" RPM");
    rpmInput->setValue(30.0);
    auto *sendRpmButton = new QPushButton("Set Motor RPM", this);
    auto *motorStatusButton = new QPushButton("Read Motor Status", this);
    statusLabel = new QLabel("Select an STM32 IP address.", this);
    motorStatusLabel = new QLabel("Motor status: unknown", this);
    motorStatusTimer = new QTimer(this);
    motorStatusTimer->setSingleShot(true);
    statusLabel->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(ipSelector);
    layout->addWidget(onButton);
    layout->addWidget(offButton);
    layout->addWidget(toggleButton);
    layout->addWidget(refreshButton);
    layout->addWidget(rpmInput);
    layout->addWidget(sendRpmButton);
    layout->addWidget(motorStatusButton);
    layout->addWidget(motorStatusLabel);
    layout->addWidget(statusLabel);
    layout->addStretch();

    setWindowTitle("STM32 LED Control");
    resize(360, 280);

    connect(onButton, &QPushButton::clicked, this, [this] { sendCommand("PB0 ON"); });
    connect(offButton, &QPushButton::clicked, this, [this] { sendCommand("PB0 OFF"); });
    connect(toggleButton, &QPushButton::clicked, this, [this] { sendCommand("PB0 TOGGLE"); });
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::requestStatus);
    connect(sendRpmButton, &QPushButton::clicked, this, [this] {
        sendMotorCommand(rpmInput->value());
    });
    connect(motorStatusButton, &QPushButton::clicked, this, &MainWindow::requestMotorStatus);
    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readReply);
    connect(motorStatusTimer, &QTimer::timeout, this, [this] {
        motorStatusLabel->setText("Motor status: no response from STM32");
    });
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

void MainWindow::sendMotorCommand(double rpm)
{
    sendCommand(QString("MOTOR %1").arg(rpm, 0, 'f', 1));
}

void MainWindow::requestMotorStatus()
{
    sendCommand("MOTOR STATUS");
    motorStatusTimer->start(1000);
}

void MainWindow::readReply()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray reply;
        reply.resize(static_cast<int>(udpSocket->pendingDatagramSize()));
        udpSocket->readDatagram(reply.data(), reply.size());

        if (reply == "OK")
            statusLabel->setText("STM32 accepted command");
        else if (reply == "PB0 ON")
            statusLabel->setText("LED status: ON");
        else if (reply == "PB0 OFF")
            statusLabel->setText("LED status: OFF");
        else if (reply.startsWith("MOTOR STATUS")) {
            motorStatusTimer->stop();
            motorStatusLabel->setText("Motor status: " + QString::fromUtf8(reply));
        }
        else
            statusLabel->setText("STM32 reply: " + QString::fromUtf8(reply));
    }
}
