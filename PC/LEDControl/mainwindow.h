#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    QWidget *createDashboardPage();
    QWidget *createConfigurationPage();
    QWidget *createDiagnosticsPage();
    QWidget *createHistoryPage();
    void sendCommand(const QString &command);
    void sendMotorCommand(double rpm);
    void requestMotorStatus();
    void requestStatus();
    void readReply();
    void addHistoryEntry(const QString &message);

    Ui::MainWindow *ui;
    class QUdpSocket *udpSocket;
    class QComboBox *ipSelector;
    class QLabel *statusLabel;
    class QLabel *motorStatusLabel;
    class QDoubleSpinBox *rpmInput;
    QTimer *motorStatusTimer;
    class QLabel *dashboardConnectionLabel;
    class QLabel *dashboardLedLabel;
    class QTextEdit *historyData;
};
#endif // MAINWINDOW_H
