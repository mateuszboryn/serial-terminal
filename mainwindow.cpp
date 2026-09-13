#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QSerialPortInfo>
#include <QMessageBox>
#include <QLabel>
#include <QTimer>
#include <QSettings>
#include <QKeyEvent>
#include <QClipboard>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serial(this)
    , m_lineStatusTimer(new QTimer(this))
    , m_statusLabel(new QLabel(this))
    , m_bytesLabel(new QLabel(this))
    , m_bytesSent(0)
    , m_bytesReceived(0)
{
    ui->setupUi(this);

    // Set initial connect button text
    ui->connectBtn->setText(tr("Connect"));

    // Status bar setup
    ui->statusbar->addWidget(m_statusLabel, 1);
    ui->statusbar->addPermanentWidget(m_bytesLabel);

    // Read-only / read-write line setup
    ui->pin_DCD_Cb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->pin_DCD_Cb->setFocusPolicy(Qt::NoFocus);
    ui->pin_DSR_Cb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->pin_DSR_Cb->setFocusPolicy(Qt::NoFocus);
    ui->pin_CTS_Cb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->pin_CTS_Cb->setFocusPolicy(Qt::NoFocus);
    ui->pin_RI_Cb->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->pin_RI_Cb->setFocusPolicy(Qt::NoFocus);

    // Event filters for port refresh on expand and console key transmission
    ui->portCB->installEventFilter(this);
    ui->console->installEventFilter(this);

    // Initialize combo boxes and refresh available ports
    initSettingsUi();
    refreshPortsList();
    loadSettings();
    updateStatusBar();

    // Signal / Slot connections
    connect(ui->connectBtn, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    connect(&m_serial, &QSerialPort::readyRead, this, &MainWindow::onReadyRead);
    connect(&m_serial, &QSerialPort::bytesWritten, this, &MainWindow::onBytesWritten);
    connect(&m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);

    connect(m_lineStatusTimer, &QTimer::timeout, this, &MainWindow::refreshLineStates);

    connect(ui->pin_DTR_Cb, &QCheckBox::toggled, this, &MainWindow::onDtrToggled);
    connect(ui->pin_RTS_Cb, &QCheckBox::toggled, this, &MainWindow::onRtsToggled);
    connect(ui->pin_TxD_Cb, &QCheckBox::toggled, this, &MainWindow::onTxdToggled);
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    closeSerialPort();
    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->portCB) {
        if (event->type() == QEvent::MouseButtonPress ||
            (event->type() == QEvent::KeyPress &&
             (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Down ||
              static_cast<QKeyEvent*>(event)->key() == Qt::Key_Space ||
              static_cast<QKeyEvent*>(event)->key() == Qt::Key_F4))) {
            refreshPortsList();
        }
    } else if (watched == ui->console) {
        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent*>(event);
            if (m_serial.isOpen()) {
                if (keyEvent->matches(QKeySequence::Paste)) {
                    const QClipboard *clipboard = QGuiApplication::clipboard();
                    if (clipboard) {
                        QByteArray data = clipboard->text().toUtf8();
                        if (!data.isEmpty()) {
                            m_serial.write(data);
                        }
                    }
                } else {
                    QByteArray data;
                    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                        data = "\r";
                    } else {
                        data = keyEvent->text().toUtf8();
                    }
                    if (!data.isEmpty()) {
                        m_serial.write(data);
                    }
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::initSettingsUi()
{
    const QList<int> baudRates = QSerialPortInfo::standardBaudRates();
    for (int rate : baudRates) {
        ui->baudRateCb->addItem(QString::number(rate), rate);
    }
    int baud9600Idx = ui->baudRateCb->findData(QSerialPort::Baud9600);
    if (baud9600Idx == -1) {
        baud9600Idx = ui->baudRateCb->findData(9600);
    }
    if (baud9600Idx != -1) {
        ui->baudRateCb->setCurrentIndex(baud9600Idx);
    }

    ui->dataBitsCb->addItem("5", QSerialPort::Data5);
    ui->dataBitsCb->addItem("6", QSerialPort::Data6);
    ui->dataBitsCb->addItem("7", QSerialPort::Data7);
    ui->dataBitsCb->addItem("8", QSerialPort::Data8);
    ui->dataBitsCb->setCurrentIndex(ui->dataBitsCb->findData(QSerialPort::Data8));

    ui->stopBitsCb->addItem("1", QSerialPort::OneStop);
    ui->stopBitsCb->addItem("1.5", QSerialPort::OneAndHalfStop);
    ui->stopBitsCb->addItem("2", QSerialPort::TwoStop);
    ui->stopBitsCb->setCurrentIndex(ui->stopBitsCb->findData(QSerialPort::OneStop));

    ui->parityCb->addItem("No", QSerialPort::NoParity);
    ui->parityCb->addItem("Even", QSerialPort::EvenParity);
    ui->parityCb->addItem("Odd", QSerialPort::OddParity);
    ui->parityCb->addItem("Mark", QSerialPort::MarkParity);
    ui->parityCb->addItem("Space", QSerialPort::SpaceParity);
    ui->parityCb->setCurrentIndex(ui->parityCb->findData(QSerialPort::NoParity));
}

void MainWindow::refreshPortsList()
{
    const QString currentPort = ui->portCB->currentText();
    ui->portCB->blockSignals(true);
    ui->portCB->clear();

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        ui->portCB->addItem(port.portName());
    }

    int index = ui->portCB->findText(currentPort);
    if (index != -1) {
        ui->portCB->setCurrentIndex(index);
    } else if (ui->portCB->count() > 0) {
        ui->portCB->setCurrentIndex(0);
    }
    ui->portCB->blockSignals(false);
}

void MainWindow::toggleConnection()
{
    if (m_serial.isOpen()) {
        closeSerialPort();
    } else {
        openSerialPort();
    }
}

void MainWindow::openSerialPort()
{
    const QString portName = ui->portCB->currentText();
    if (portName.isEmpty()) {
        QMessageBox::critical(this, tr("Critical Error"), tr("No serial port selected."));
        return;
    }

    m_serial.setPortName(portName);
    m_serial.setBaudRate(ui->baudRateCb->currentData().toInt());
    m_serial.setDataBits(static_cast<QSerialPort::DataBits>(ui->dataBitsCb->currentData().toInt()));
    m_serial.setParity(static_cast<QSerialPort::Parity>(ui->parityCb->currentData().toInt()));
    m_serial.setStopBits(static_cast<QSerialPort::StopBits>(ui->stopBitsCb->currentData().toInt()));
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial.open(QIODevice::ReadWrite)) {
        ui->connectBtn->setText(tr("Disconnect"));
        ui->portCB->setEnabled(false);
        ui->baudRateCb->setEnabled(false);
        ui->dataBitsCb->setEnabled(false);
        ui->parityCb->setEnabled(false);
        ui->stopBitsCb->setEnabled(false);

        m_bytesSent = 0;
        m_bytesReceived = 0;

        m_serial.setDataTerminalReady(ui->pin_DTR_Cb->isChecked());
        m_serial.setRequestToSend(ui->pin_RTS_Cb->isChecked());
        m_serial.setBreakEnabled(ui->pin_TxD_Cb->isChecked());

        m_lineStatusTimer->start(100);
        refreshLineStates();
        updateStatusBar();
    } else {
        // Critical error alert
        QMessageBox::critical(this, tr("Critical Error"), tr("Cannot open port %1:\n%2").arg(portName, m_serial.errorString()));
        updateStatusBar();
    }
}

void MainWindow::closeSerialPort()
{
    if (m_serial.isOpen()) {
        m_serial.close();
    }
    m_lineStatusTimer->stop();

    ui->connectBtn->setText(tr("Connect"));
    ui->portCB->setEnabled(true);
    ui->baudRateCb->setEnabled(true);
    ui->dataBitsCb->setEnabled(true);
    ui->parityCb->setEnabled(true);
    ui->stopBitsCb->setEnabled(true);

    ui->pin_DCD_Cb->setChecked(false);
    ui->pin_DSR_Cb->setChecked(false);
    ui->pin_CTS_Cb->setChecked(false);
    ui->pin_RI_Cb->setChecked(false);

    updateStatusBar();
}

void MainWindow::onReadyRead()
{
    const QByteArray data = m_serial.readAll();
    m_bytesReceived += data.size();
    ui->console->moveCursor(QTextCursor::End);
    ui->console->insertPlainText(QString::fromLocal8Bit(data));
    ui->console->moveCursor(QTextCursor::End);
    updateStatusBar();
}

void MainWindow::onBytesWritten(qint64 bytes)
{
    m_bytesSent += bytes;
    updateStatusBar();
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    if (error == QSerialPort::ResourceError ||
        error == QSerialPort::PermissionError ||
        error == QSerialPort::DeviceNotFoundError ||
        error == QSerialPort::OpenError) {
        const QString errorMsg = m_serial.errorString();
        if (m_serial.isOpen()) {
            closeSerialPort();
            QMessageBox::critical(this, tr("Critical Error"), tr("Serial port error: %1").arg(errorMsg));
        }
        updateStatusBar();
    } else {
        // Recoverable error: signal in status bar for 10 seconds (10,000 ms)
        ui->statusbar->showMessage(tr("Warning: %1").arg(m_serial.errorString()), 10000);
    }
}

void MainWindow::refreshLineStates()
{
    if (m_serial.isOpen()) {
        const QSerialPort::PinoutSignals pinSignals = m_serial.pinoutSignals();
        ui->pin_DCD_Cb->setChecked(pinSignals.testFlag(QSerialPort::DataCarrierDetectSignal));
        ui->pin_DSR_Cb->setChecked(pinSignals.testFlag(QSerialPort::DataSetReadySignal));
        ui->pin_CTS_Cb->setChecked(pinSignals.testFlag(QSerialPort::ClearToSendSignal));
        ui->pin_RI_Cb->setChecked(pinSignals.testFlag(QSerialPort::RingIndicatorSignal));
    }
}

void MainWindow::onDtrToggled(bool checked)
{
    if (m_serial.isOpen()) {
        m_serial.setDataTerminalReady(checked);
    }
}

void MainWindow::onRtsToggled(bool checked)
{
    if (m_serial.isOpen()) {
        m_serial.setRequestToSend(checked);
    }
}

void MainWindow::onTxdToggled(bool checked)
{
    if (m_serial.isOpen()) {
        m_serial.setBreakEnabled(checked);
    }
}

void MainWindow::updateStatusBar()
{
    if (m_serial.isOpen()) {
        m_statusLabel->setText(tr("Connected to %1 (%2, %3, %4, %5)")
                               .arg(m_serial.portName())
                               .arg(m_serial.baudRate())
                               .arg(ui->dataBitsCb->currentText())
                               .arg(ui->parityCb->currentText())
                               .arg(ui->stopBitsCb->currentText()));
    } else {
        m_statusLabel->setText(tr("Disconnected"));
    }

    m_bytesLabel->setText(tr("Sent: %1 bytes | Received: %2 bytes")
                          .arg(m_bytesSent)
                          .arg(m_bytesReceived));
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("portName", ui->portCB->currentText());
    settings.setValue("baudRate", ui->baudRateCb->currentData());
    settings.setValue("dataBits", ui->dataBitsCb->currentData());
    settings.setValue("parity", ui->parityCb->currentData());
    settings.setValue("stopBits", ui->stopBitsCb->currentData());
    settings.setValue("dtr", ui->pin_DTR_Cb->isChecked());
    settings.setValue("rts", ui->pin_RTS_Cb->isChecked());
    settings.setValue("txd", ui->pin_TxD_Cb->isChecked());
}

void MainWindow::loadSettings()
{
    QSettings settings;
    const QString portName = settings.value("portName").toString();
    if (!portName.isEmpty()) {
        int idx = ui->portCB->findText(portName);
        if (idx != -1) {
            ui->portCB->setCurrentIndex(idx);
        }
    }

    const QVariant baudRate = settings.value("baudRate");
    if (baudRate.isValid()) {
        int idx = ui->baudRateCb->findData(baudRate);
        if (idx != -1) {
            ui->baudRateCb->setCurrentIndex(idx);
        }
    }

    const QVariant dataBits = settings.value("dataBits");
    if (dataBits.isValid()) {
        int idx = ui->dataBitsCb->findData(dataBits);
        if (idx != -1) {
            ui->dataBitsCb->setCurrentIndex(idx);
        }
    }

    const QVariant parity = settings.value("parity");
    if (parity.isValid()) {
        int idx = ui->parityCb->findData(parity);
        if (idx != -1) {
            ui->parityCb->setCurrentIndex(idx);
        }
    }

    const QVariant stopBits = settings.value("stopBits");
    if (stopBits.isValid()) {
        int idx = ui->stopBitsCb->findData(stopBits);
        if (idx != -1) {
            ui->stopBitsCb->setCurrentIndex(idx);
        }
    }

    if (settings.contains("dtr")) {
        ui->pin_DTR_Cb->setChecked(settings.value("dtr").toBool());
    }
    if (settings.contains("rts")) {
        ui->pin_RTS_Cb->setChecked(settings.value("rts").toBool());
    }
    if (settings.contains("txd")) {
        ui->pin_TxD_Cb->setChecked(settings.value("txd").toBool());
    }
}

