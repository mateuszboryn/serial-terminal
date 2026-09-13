#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
class QLabel;
class QTimer;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void toggleConnection();
    void onReadyRead();
    void onBytesWritten(qint64 bytes);
    void handleError(QSerialPort::SerialPortError error);
    void refreshLineStates();
    void onDtrToggled(bool checked);
    void onRtsToggled(bool checked);
    void onTxdToggled(bool checked);

private:
    Ui::MainWindow *ui;
    QSerialPort m_serial;
    QTimer *m_lineStatusTimer;
    QLabel *m_statusLabel;
    QLabel *m_bytesLabel;
    qint64 m_bytesSent;
    qint64 m_bytesReceived;

    void initSettingsUi();
    void refreshPortsList();
    void openSerialPort();
    void closeSerialPort();
    void updateStatusBar();
    void saveSettings();
    void loadSettings();
};
#endif // MAINWINDOW_H
