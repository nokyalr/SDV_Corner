#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHostAddress>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QUdpSocket>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QCheckBox>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QWidget>
#include <QDebug>          // for debugging (optional)
#include <QtGlobal>        // for qAsConst

namespace {
constexpr quint16 kLedCommandPort = 5055;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , udpSocket(new QUdpSocket(this))
{
    ui->setupUi(this);

    // ===== MAIN WIDGET WITH SCROLL AREA =====
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto *containerWidget = new QWidget(this);

    auto *mainLayout = new QVBoxLayout(containerWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // ===== HEADER =====
    auto *title = new QLabel("Multi-STM32 Controller", this);
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    mainLayout->addWidget(title);

    // ===== DEVICE SELECTION =====
    auto *deviceGroup = new QGroupBox("Select STM32 Devices", this);
    auto *deviceLayout = new QVBoxLayout(deviceGroup);

    // Select/Deselect All buttons
    auto *selectAllLayout = new QHBoxLayout();
    auto *selectAllBtn = new QPushButton("Select All", this);
    auto *deselectAllBtn = new QPushButton("Deselect All", this);
    selectAllLayout->addWidget(selectAllBtn);
    selectAllLayout->addWidget(deselectAllBtn);
    selectAllLayout->addStretch();
    deviceLayout->addLayout(selectAllLayout);

    // Device list with checkboxes
    deviceList = new QListWidget(this);
    deviceList->setSelectionMode(QAbstractItemView::MultiSelection);
    QStringList devices = {"10.252.62.51 (Corner 1)",
                           "10.252.62.52 (Corner 2)",
                           "10.252.62.53 (Corner 3)",
                           "10.252.62.54 (Corner 4)"};
    for (const QString &dev : devices) {
        auto *item = new QListWidgetItem(dev);
        item->setCheckState(Qt::Checked);
        deviceList->addItem(item);
    }
    deviceList->setMaximumHeight(100);
    deviceLayout->addWidget(deviceList);
    mainLayout->addWidget(deviceGroup);

    // ===== STATUS BAR =====
    statusLabel = new QLabel("Ready - Select devices to control", this);
    statusLabel->setWordWrap(true);
    mainLayout->addWidget(statusLabel);

    // ===== LED CONTROL =====
    auto *ledGroup = new QGroupBox("LED Control (PB0)", this);
    auto *ledLayout = new QHBoxLayout(ledGroup);

    auto *onButton = new QPushButton("ON", this);
    onButton->setFixedWidth(60);
    auto *offButton = new QPushButton("OFF", this);
    offButton->setFixedWidth(60);
    auto *toggleButton = new QPushButton("TOGGLE", this);
    toggleButton->setFixedWidth(70);
    auto *refreshButton = new QPushButton("⟳", this);
    refreshButton->setFixedWidth(40);
    refreshButton->setToolTip("Refresh Status");

    ledLayout->addWidget(onButton);
    ledLayout->addWidget(offButton);
    ledLayout->addWidget(toggleButton);
    ledLayout->addWidget(refreshButton);
    ledLayout->addStretch();
    mainLayout->addWidget(ledGroup);

    // ===== MOTOR CONTROL =====
    auto *motorGroup = new QGroupBox("Motor Control", this);
    auto *motorLayout = new QGridLayout(motorGroup);

    auto *rpmLabel = new QLabel("Set RPM:", this);
    rpmInput = new QDoubleSpinBox(this);
    rpmInput->setRange(-120.0, 120.0);
    rpmInput->setDecimals(1);
    rpmInput->setSuffix(" RPM");
    rpmInput->setValue(30.0);
    rpmInput->setFixedWidth(120);

    auto *sendRpmButton = new QPushButton("Send", this);
    sendRpmButton->setFixedWidth(60);
    auto *motorStatusButton = new QPushButton("Read Status", this);

    motorStatusLabel = new QLabel("Status: unknown", this);

    motorLayout->addWidget(rpmLabel, 0, 0);
    motorLayout->addWidget(rpmInput, 0, 1);
    motorLayout->addWidget(sendRpmButton, 0, 2);
    motorLayout->addWidget(motorStatusButton, 0, 3);
    motorLayout->addWidget(motorStatusLabel, 1, 0, 1, 4);
    mainLayout->addWidget(motorGroup);

    // ===== STEPPER/STEERING CONTROL =====
    auto *stepperGroup = new QGroupBox("Steering Control", this);
    auto *stepperLayout = new QGridLayout(stepperGroup);

    auto *angleLabel = new QLabel("Set Angle:", this);
    angleInput = new QDoubleSpinBox(this);
    angleInput->setRange(-40.0, 40.0);
    angleInput->setDecimals(1);
    angleInput->setSuffix(" deg");
    angleInput->setValue(0.0);
    angleInput->setFixedWidth(120);

    auto *sendAngleButton = new QPushButton("Send", this);
    sendAngleButton->setFixedWidth(60);
    auto *stepperStatusButton = new QPushButton("Read Status", this);

    stepperStatusLabel = new QLabel("Status: unknown", this);

    stepperLayout->addWidget(angleLabel, 0, 0);
    stepperLayout->addWidget(angleInput, 0, 1);
    stepperLayout->addWidget(sendAngleButton, 0, 2);
    stepperLayout->addWidget(stepperStatusButton, 0, 3);
    stepperLayout->addWidget(stepperStatusLabel, 1, 0, 1, 4);
    mainLayout->addWidget(stepperGroup);

    // ===== QUICK COMMANDS =====
    auto *quickGroup = new QGroupBox("Quick Commands", this);
    auto *quickLayout = new QHBoxLayout(quickGroup);

    auto *motorAutoBtn = new QPushButton("Motor Auto", this);
    auto *motorStopBtn = new QPushButton("Motor Stop", this);
    auto *stepperAutoBtn = new QPushButton("Steering Auto", this);
    auto *stepperStopBtn = new QPushButton("Steering Stop", this);

    quickLayout->addWidget(motorAutoBtn);
    quickLayout->addWidget(motorStopBtn);
    quickLayout->addWidget(stepperAutoBtn);
    quickLayout->addWidget(stepperStopBtn);
    quickLayout->addStretch();
    mainLayout->addWidget(quickGroup);

    // ===== INDIVIDUAL STATUS TABLE =====
    auto *statusGroup = new QGroupBox("Device Status", this);
    auto *statusLayout = new QVBoxLayout(statusGroup);

    statusTable = new QTableWidget(this);
    statusTable->setColumnCount(4);
    statusTable->setHorizontalHeaderLabels({"Device", "LED", "Motor (RPM)", "Steering (deg)"});
    statusTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    statusTable->setMaximumHeight(150);
    statusTable->setAlternatingRowColors(true);
    statusLayout->addWidget(statusTable);
    mainLayout->addWidget(statusGroup);

    // ===== SET SCROLL AREA =====
    scrollArea->setWidget(containerWidget);
    setCentralWidget(scrollArea);

    // ===== TIMERS =====
    motorStatusTimer = new QTimer(this);
    motorStatusTimer->setSingleShot(true);

    stepperStatusTimer = new QTimer(this);
    stepperStatusTimer->setSingleShot(true);

    // Initialize status table
    initializeStatusTable();

    // ===== CONNECTIONS =====
    connect(selectAllBtn, &QPushButton::clicked, this, &MainWindow::selectAllDevices);
    connect(deselectAllBtn, &QPushButton::clicked, this, &MainWindow::deselectAllDevices);

    connect(onButton, &QPushButton::clicked, this, [this] { sendCommandToAll("PB0 ON"); });
    connect(offButton, &QPushButton::clicked, this, [this] { sendCommandToAll("PB0 OFF"); });
    connect(toggleButton, &QPushButton::clicked, this, [this] { sendCommandToAll("PB0 TOGGLE"); });
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::requestStatusAll);

    connect(sendRpmButton, &QPushButton::clicked, this, [this] {
        sendMotorCommandToAll(rpmInput->value());
    });
    connect(motorStatusButton, &QPushButton::clicked, this, &MainWindow::requestMotorStatusAll);

    connect(sendAngleButton, &QPushButton::clicked, this, [this] {
        sendStepperCommandToAll(angleInput->value());
    });
    connect(stepperStatusButton, &QPushButton::clicked, this, &MainWindow::requestStepperStatusAll);

    connect(motorAutoBtn, &QPushButton::clicked, this, [this] {
        sendCommandToAll("MOTOR AUTO");
        statusLabel->setText("Motor set to Auto mode for all selected devices");
    });
    connect(motorStopBtn, &QPushButton::clicked, this, [this] {
        sendCommandToAll("MOTOR STOP");
        statusLabel->setText("Motor stopped for all selected devices");
    });
    connect(stepperAutoBtn, &QPushButton::clicked, this, [this] {
        sendCommandToAll("STEPPER AUTO");
        statusLabel->setText("Steering set to Auto mode for all selected devices");
    });
    connect(stepperStopBtn, &QPushButton::clicked, this, [this] {
        sendCommandToAll("STEPPER STOP");
        statusLabel->setText("Steering stopped for all selected devices");
    });

    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readReply);

    connect(motorStatusTimer, &QTimer::timeout, this, [this] {
        motorStatusLabel->setText("Status: timeout");
    });

    connect(stepperStatusTimer, &QTimer::timeout, this, [this] {
        stepperStatusLabel->setText("Status: timeout");
    });

    // ===== WINDOW SETTINGS =====
    setWindowTitle("Multi-STM32 Controller");
    resize(550, 700);
    setMinimumSize(450, 600);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ===== IMPLEMENTATION OF SLOTS =====

void MainWindow::requestStatus()
{
    sendCommandToAll("PB0 STATUS");
}

void MainWindow::requestMotorStatus()
{
    sendCommandToAll("MOTOR STATUS");
    motorStatusLabel->setText("Status: waiting...");
    motorStatusTimer->start(1000);
}

void MainWindow::requestStepperStatus()
{
    sendCommandToAll("STEPPER STATUS");
    stepperStatusLabel->setText("Status: waiting...");
    stepperStatusTimer->start(1000);
}

// ===== IMPLEMENTATION OF METHODS =====

void MainWindow::initializeStatusTable()
{
    statusTable->setRowCount(4);
    QStringList devices = {"10.252.62.51", "10.252.62.52", "10.252.62.53", "10.252.62.54"};

    for (int i = 0; i < devices.size(); i++) {
        statusTable->setItem(i, 0, new QTableWidgetItem(devices[i]));
        statusTable->setItem(i, 1, new QTableWidgetItem("Unknown"));
        statusTable->setItem(i, 2, new QTableWidgetItem("Unknown"));
        statusTable->setItem(i, 3, new QTableWidgetItem("Unknown"));
    }
}

QStringList MainWindow::getSelectedDevices()
{
    QStringList devices;
    for (int i = 0; i < deviceList->count(); i++) {
        auto *item = deviceList->item(i);
        if (item->checkState() == Qt::Checked) {
            // Extract IP from the item text (e.g., "10.252.62.51 (Corner 1)")
            QString text = item->text();
            QString ip = text.split(" ").first();
            devices.append(ip);
        }
    }
    return devices;
}

void MainWindow::sendCommandToAll(const QString &command)
{
    QStringList devices = getSelectedDevices();
    if (devices.isEmpty()) {
        statusLabel->setText("❌ No devices selected!");
        return;
    }

    // Use qAsConst to prevent detach
    for (const QString &ip : std::as_const(devices)) {
        QHostAddress boardAddress(ip);
        if (!boardAddress.isNull()) {
            udpSocket->writeDatagram(command.toUtf8(), boardAddress, kLedCommandPort);
        }
    }
    // Use multi-arg in one call (Qt 5.14+ supports int directly, but we convert for safety)
    statusLabel->setText(QString("📤 Sent '%1' to %2 devices")
                             .arg(command, QString::number(devices.size())));
}

void MainWindow::sendMotorCommandToAll(double rpm)
{
    QString command = QString("MOTOR %1").arg(rpm, 0, 'f', 1);
    sendCommandToAll(command);
}

void MainWindow::sendStepperCommandToAll(double angle)
{
    QString command = QString("STEPPER %1").arg(angle, 0, 'f', 1);
    sendCommandToAll(command);
}

void MainWindow::requestStatusAll()
{
    sendCommandToAll("PB0 STATUS");
}

void MainWindow::requestMotorStatusAll()
{
    sendCommandToAll("MOTOR STATUS");
    motorStatusLabel->setText("Status: waiting...");
    motorStatusTimer->start(1000);
}

void MainWindow::requestStepperStatusAll()
{
    sendCommandToAll("STEPPER STATUS");
    stepperStatusLabel->setText("Status: waiting...");
    stepperStatusTimer->start(1000);
}

void MainWindow::selectAllDevices()
{
    for (int i = 0; i < deviceList->count(); i++) {
        deviceList->item(i)->setCheckState(Qt::Checked);
    }
    statusLabel->setText("All devices selected");
}

void MainWindow::deselectAllDevices()
{
    for (int i = 0; i < deviceList->count(); i++) {
        deviceList->item(i)->setCheckState(Qt::Unchecked);
    }
    statusLabel->setText("All devices deselected");
}

void MainWindow::updateDeviceStatus(const QString &ip, const QString &type, const QString &value)
{
    // Find the row for this IP
    for (int row = 0; row < statusTable->rowCount(); row++) {
        QTableWidgetItem *item = statusTable->item(row, 0);
        if (item && item->text() == ip) {
            if (type == "LED") {
                statusTable->item(row, 1)->setText(value);
            } else if (type == "MOTOR") {
                statusTable->item(row, 2)->setText(value);
            } else if (type == "STEPPER") {
                statusTable->item(row, 3)->setText(value);
            }
            break;
        }
    }
}

void MainWindow::sendCommand(const QString &command)
{
    // Send to single device (for backward compatibility)
    QStringList devices = getSelectedDevices();
    if (devices.isEmpty()) {
        statusLabel->setText("❌ No devices selected!");
        return;
    }

    // Send to first selected device only
    QHostAddress boardAddress(devices.first());
    if (!boardAddress.isNull()) {
        udpSocket->writeDatagram(command.toUtf8(), boardAddress, kLedCommandPort);
        statusLabel->setText("📤 Sent: " + command + " to " + devices.first());
    }
}

void MainWindow::sendMotorCommand(double rpm)
{
    sendCommand(QString("MOTOR %1").arg(rpm, 0, 'f', 1));
}

void MainWindow::sendStepperCommand(double angle)
{
    sendCommand(QString("STEPPER %1").arg(angle, 0, 'f', 1));
}

// ============================================================
//  readReply() – fully fixed with IP parsing and trimmed()
// ============================================================
void MainWindow::readReply()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray reply;
        reply.resize(static_cast<int>(udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        udpSocket->readDatagram(reply.data(), reply.size(), &sender, &senderPort);

        // --- Robust IP extraction ---
        QString senderIP;
        // Try to get IPv4 address (handles IPv6-mapped addresses)
        quint32 ipv4 = sender.toIPv4Address();
        if (ipv4 != 0) {
            senderIP = QHostAddress(ipv4).toString();
        } else {
            // Fallback: if it's a pure IPv6, just use toString()
            senderIP = sender.toString();
            // Remove any "::ffff:" prefix if present
            if (senderIP.startsWith("::ffff:")) {
                senderIP = senderIP.mid(7);
            }
        }
        // Remove port if accidentally appended (just in case)
        if (senderIP.contains(":")) {
            senderIP = senderIP.section(':', 0, 0);
        }

        // --- Trim the reply to remove leading/trailing whitespace ---
        QByteArray trimmedReply = reply.trimmed();

        // Debug output (optional, remove if not needed)
        qDebug() << "Received from" << senderIP << ":" << trimmedReply;

        // --- Process known replies ---
        if (trimmedReply == "PB0 ON") {
            updateDeviceStatus(senderIP, "LED", "ON");
            statusLabel->setText(QString("💡 %1 LED: ON").arg(senderIP));
        }
        else if (trimmedReply == "PB0 OFF") {
            updateDeviceStatus(senderIP, "LED", "OFF");
            statusLabel->setText(QString("💡 %1 LED: OFF").arg(senderIP));
        }
        else if (trimmedReply.startsWith("MOTOR STATUS")) {
            motorStatusTimer->stop();
            QString status = QString::fromUtf8(trimmedReply);
            motorStatusLabel->setText(status);

            // Parse RPM value from "MOTOR STATUS ref actual"
            QStringList parts = status.split(" ");
            if (parts.size() >= 3) {
                updateDeviceStatus(senderIP, "MOTOR", parts[2]);
            }
        }
        else if (trimmedReply.startsWith("STEPPER STATUS")) {
            stepperStatusTimer->stop();
            QString status = QString::fromUtf8(trimmedReply);
            stepperStatusLabel->setText(status);

            // Parse angle from "STEPPER STATUS ref actual"
            QStringList parts = status.split(" ");
            if (parts.size() >= 3) {
                updateDeviceStatus(senderIP, "STEPPER", parts[2]);
            }
        }
        else if (trimmedReply == "OK") {
            statusLabel->setText(QString("✅ %1: Command accepted").arg(senderIP));
        }
        else {
            statusLabel->setText(QString("📨 %1: %2").arg(senderIP, QString::fromUtf8(trimmedReply)));
        }
    }
}