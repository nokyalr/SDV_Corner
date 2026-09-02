#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QComboBox;
class QLabel;
class QDoubleSpinBox;
class QUdpSocket;
class QListWidget;
class QTableWidget;
class QTextEdit;
class QScrollArea;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void readReply();
    void requestStatus();
    void requestMotorStatus();
    void requestStepperStatus();

private:
    // UI Components
    Ui::MainWindow *ui;
    QUdpSocket *udpSocket;

    // Device Selection
    QListWidget *deviceList;

    // Status
    QLabel *statusLabel;
    QTableWidget *statusTable;

    // Motor Control
    QDoubleSpinBox *rpmInput;
    QLabel *motorStatusLabel;
    QTimer *motorStatusTimer;

    // Stepper Control
    QDoubleSpinBox *angleInput;
    QLabel *stepperStatusLabel;
    QTimer *stepperStatusTimer;

    // Methods
    void initializeStatusTable();
    QStringList getSelectedDevices();
    void sendCommandToAll(const QString &command);
    void sendMotorCommandToAll(double rpm);
    void sendStepperCommandToAll(double angle);
    void requestStatusAll();
    void requestMotorStatusAll();
    void requestStepperStatusAll();
    void selectAllDevices();
    void deselectAllDevices();
    void updateDeviceStatus(const QString &ip, const QString &type, const QString &value);

    // Original methods (kept for compatibility)
    void sendCommand(const QString &command);
    void sendMotorCommand(double rpm);
    void sendStepperCommand(double angle);
};

#endif // MAINWINDOW_H