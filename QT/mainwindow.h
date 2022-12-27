#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDebug>
#include <QCanBusDevice>
#include <QMainWindow>
#include <QTimer>
#include <QFile>

class data_frame;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startScan();
    void starteeprom();
    void restarteeprom();
    void processReceivedFrames();
    void sendFrame(const QCanBusFrame &frame) const;
    void processErrors(QCanBusDevice::CanBusError) const;

private:
    std::unique_ptr<QCanBusDevice> m_device;
    QTimer *timer;
    QTimer *restart_timer;

    QByteArray protocol_versie,module_adres,EE_IO_block,I_max_block,O_max_block;
    int run_at=0;
    bool has_files=false;
    bool has_to_reset=false;

     uint frameId;
    int adres;
    QString eeprom_data;
    QString eeprom_data_write;
    bool process_Frame_scan(const QCanBusFrame &frame);
    bool process_Frame_eeprom(const QCanBusFrame &frame);
    uint get_frameId();
};

#endif // MAINWINDOW_H
