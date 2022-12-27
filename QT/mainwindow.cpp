#include "mainwindow.h"

#include <QPushButton>
#include <QWidget>
#include <QGridLayout>
#include <QMdiArea>
#include <QPointer>
#include <QCanBus>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QPointer<QWidget> widget= new QWidget(this);
    QPointer<QGridLayout> gridGroupBox = new QGridLayout();
    QPointer<QPushButton> start_scan_pb = new QPushButton(tr("start scan"));
    connect(start_scan_pb,SIGNAL(pressed()),this,SLOT(startScan()));
    QPointer<QMdiArea> mdiArea = new QMdiArea();
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(mdiArea);
    connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(updateMenus()));
        gridGroupBox->addWidget(start_scan_pb,0,0,1,1);
        widget->setLayout(gridGroupBox);
        widget->setWindowTitle("start");
        mdiArea->addSubWindow(widget);
        mdiArea->setViewMode(QMdiArea::TabbedView);
        widget->show();
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(starteeprom()));
    restart_timer = new QTimer(this);
    connect(restart_timer, SIGNAL(timeout()), this, SLOT(restarteeprom()));

    QString errorString;
     const QList<QCanBusDeviceInfo> devices = QCanBus::instance()->availableDevices(
         QStringLiteral("socketcan"), &errorString);
     if (!errorString.isEmpty())
         qDebug() << errorString;
     foreach (QCanBusDeviceInfo var, devices) {
         qDebug()<< var.name()<<var.description();
     }
//     m_device = QCanBus::instance()->createDevice("socketcan","can0");


     m_device.reset(QCanBus::instance()->createDevice("socketcan","can0",
                                                         &errorString));
     if (!m_device) {
         qDebug() << tr("Error creating device '%1', reason: '%2'")
                           .arg("socketcan").arg(errorString);
         return;
     }
     connect(m_device.get(), &QCanBusDevice::errorOccurred,
             this, &MainWindow::processErrors);
     connect(m_device.get(), &QCanBusDevice::framesReceived,
             this, &MainWindow::processReceivedFrames);
     m_device->connectDevice();
}

MainWindow::~MainWindow()
{
    if(m_device->state() == QCanBusDevice::ConnectedState)
    {
        m_device->disconnectDevice();
    }

}

void MainWindow::startScan()
{
    module_adres.clear();
    EE_IO_block.clear();
    I_max_block.clear();
    O_max_block.clear();
    run_at=0;

    sendFrame(QCanBusFrame(0x1ff,QByteArray()));
}

void MainWindow::starteeprom()
{
    timer->stop();
    restart_timer->stop();
    has_files=false;
    if(module_adres.size()>run_at)
    {
        qDebug()<<"-------------------";
        qDebug()<<"starteeprom"<<run_at<<QString::number(module_adres.at(run_at),16);

        /* load data from files
         * 3 files:  main, in, out
         * main: DDR PORD   eg: 11_main.txt
         * in: de in list   eg: 11_in.txt
         * out: de out list eg: 11_out.txt
         */
        QString main,in,out;
        {
            bool is_true_file=false;
            QFile file("data/"+QString::number(module_adres.at(run_at),16)+"_main.txt");
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
                while (!file.atEnd()) {
                    QString temp =file.readLine();
                    if(!is_true_file){
                        if(temp.contains(QString::number(module_adres.at(run_at),16)+"_main.txt"))
                            is_true_file=true;
                    }
                    if(temp.indexOf('#')!=-1)temp.truncate(temp.indexOf('#'));//remove info
                    main.append(temp);
                }
                has_files=true;
            }
            if(!is_true_file){
                qWarning()<<QString::number(module_adres.at(run_at),16)+"_main.txt is missing in txt is de file configerd?";
                has_files=false;
            }
        }
        {
            bool is_true_file=false;
            QFile file("data/"+QString::number(module_adres.at(run_at),16)+"_in.txt");
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
                while (!file.atEnd()) {
                    QString temp =file.readLine();
                    if(!is_true_file){
                        if(temp.contains(QString::number(module_adres.at(run_at),16)+"_in.txt"))
                            is_true_file=true;
                    }
                    if(temp.indexOf('#')!=-1)temp.truncate(temp.indexOf('#'));//remove info
                    in.append(temp);
                }
            } else {
                has_files=false;
            }
            if(!is_true_file){
                qWarning()<<QString::number(module_adres.at(run_at),16)+"_in.txt is missing in txt is de file configerd?";
                has_files=false;
            }
        }
        {
            bool is_true_file=false;
            QFile file("data/"+QString::number(module_adres.at(run_at),16)+"_out.txt");
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
                while (!file.atEnd()) {
                    QString temp =file.readLine();
                    if(!is_true_file){
                        if(temp.contains(QString::number(module_adres.at(run_at),16)+"_out.txt"))
                            is_true_file=true;
                    }
                    if(temp.indexOf('#')!=-1)temp.truncate(temp.indexOf('#'));//remove info
                    out.append(temp);
                }
            } else {
                has_files=false;
            }
            if(!is_true_file){
                qWarning()<<QString::number(module_adres.at(run_at),16)+"_out.txt is missing in txt is de file configerd?";
                has_files=false;
            }
        }
        main.remove(" ");
        main.remove("\n");
        in.remove(" ");
        in.remove("\n");
        out.remove(" ");
        out.remove("\n");

        main.resize(EE_IO_block.at(run_at)*2,'F'); //fill met 0xFF is de standaard in de eeprom
        in.resize(I_max_block.at(run_at)*2*5,'F'); //fill met 0xFF is de standaard in de eeprom
        out.resize(O_max_block.at(run_at)*2*3,'F'); //fill met 0xFF is de standaard in de eeprom
        eeprom_data_write.clear();
        eeprom_data_write=main+in+out;
        qDebug()<<"size of main in out eeprom"<<main.size()<<in.size()<<out.size()<<eeprom_data_write.size();
//        qDebug()<<"EE_IO_block:"<<EE_IO_block.at(run_at)*2;
//        qDebug()<<main;
//        qDebug()<<"I_max_block:"<<I_max_block.at(run_at)*2*5;
//        qDebug()<<in;
//        qDebug()<<"O_max_block:"<<O_max_block.at(run_at)*2*3;
//        qDebug()<<out;
//        qDebug()<<"=>"<<eeprom_data_write.size();
//        qDebug()<<eeprom_data_write;
//        qDebug()<<"__________________";
        frameId = get_frameId();
        adres=0;
        QString data;
        data.setNum(adres);
        data=data.rightJustified(4,'0');
//        data.prepend("0102");
        data.prepend("0101");
//        data.append("00000000");
        data.append(eeprom_data_write.mid(adres,8));

           //  com. adrs |-data-|
    //     data ="0101 0000 00000000";
        const QByteArray payload =QByteArray::fromHex(data.remove(QLatin1Char(' ')).toLatin1());

//        qDebug()<<payload<<data;
        QCanBusFrame frame = QCanBusFrame(frameId, payload);
        sendFrame(frame);
        restart_timer->start(5000);
    } else {
        qDebug()<<"- end ------------------";
    }

}

void MainWindow::restarteeprom()
{
    QString data;
    data.setNum(adres,16);
    data=data.rightJustified(4,'0');
    data.prepend("0101");
    data.append(eeprom_data_write.mid(adres*2,8));
    const QByteArray payload =QByteArray::fromHex(data.remove(QLatin1Char(' ')).toLatin1());

    QCanBusFrame frame = QCanBusFrame(frameId, payload);
    sendFrame(frame);
}

static QString frameFlags(const QCanBusFrame &frame)
{
    QString result = QLatin1String(" --- ");

    if (frame.hasBitrateSwitch())
        result[1] = QLatin1Char('B');
    if (frame.hasErrorStateIndicator())
        result[2] = QLatin1Char('E');
    if (frame.hasLocalEcho())
        result[3] = QLatin1Char('L');

    return result;
}

void MainWindow::processReceivedFrames()
{
    if (!m_device)
        return;

    while (m_device->framesAvailable()) {
        const QCanBusFrame frame = m_device->readFrame();
        QString view;
        if (frame.frameType() == QCanBusFrame::ErrorFrame)
            view = m_device->interpretErrorFrame(frame);
        else
            view = frame.toString();

        const QString time = QString::fromLatin1("%1.%2  ")
                .arg(frame.timeStamp().seconds(), 10, 10, QLatin1Char(' '))
                .arg(frame.timeStamp().microSeconds() / 100, 4, 10, QLatin1Char('0'));

        const QString flags = frameFlags(frame);

        if(process_Frame_eeprom(frame)){}
        else if(process_Frame_scan(frame)){}
        else {qDebug() << time + flags +"|"+ view;}
    }
}

void MainWindow::sendFrame(const QCanBusFrame &frame) const
{
    if (!m_device)
        return;
    m_device->writeFrame(frame);
}

void MainWindow::processErrors(QCanBusDevice::CanBusError) const{}

bool MainWindow::process_Frame_scan(const QCanBusFrame &frame)
{
    bool is_Frame=false;
    if(frame.payload().size()==8&&frame.frameId()>16777216&&frame.frameId()<16842751){
        is_Frame=true;
        //01000001   [8]  01 00 21 64 FF FF 00 00
        //frame.payload().at(0) MICROCONTROLLER_TYPE
        //frame.payload().at(1) PROTOCOL_VERSIE
        //frame.payload().at(2) module_adres
        //frame.payload().at(3) EE_IO_block
        //frame.payload().at(4) I_max_block
        //frame.payload().at(5) O_max_block
        //frame.payload().at(6) data1 == 00
        //frame.payload().at(7) data2 == 00
        protocol_versie.append(frame.payload().at(1));
        module_adres.append(frame.payload().at(2));
        EE_IO_block.append(frame.payload().at(3));
        I_max_block.append(frame.payload().at(4));
        O_max_block.append(frame.payload().at(5));

        timer->start(1000);
    }

    return is_Frame;
}

bool MainWindow::process_Frame_eeprom(const QCanBusFrame &frame)
{
    bool is_Frame=false;
//    if(frame.payload().size()==8&&frame.frameId()>1024&&frame.frameId()<1280)
    if(frame.payload().size()==8&&frame.frameId()==frameId)
    {
        if(frame.payload().at(0)==1&&frame.payload().at(1)==3)
        {
            //data read EEPROM
            is_Frame=true;
            restart_timer->start(5000);

            adres=(frame.payload().at(2)<<8)|frame.payload().at(3);
//            qDebug()<<"EEPROM"<<adres<<frame.payload().mid(2,2).toHex()<<frame.payload().mid(4,4).toHex();
            bool ok=false;
            if(QString(frame.payload().mid(2,2).toHex()).toInt(&ok,16)==adres){
                bool resend_payload=false;
                if(frame.payload().mid(4,4).toHex().toUpper()!=eeprom_data_write.mid(adres*2,8).toUpper())
                {
                    qDebug()<<" at"<<adres<<"diff µc:"<<frame.payload().mid(4,4).toHex().toUpper()<<"on file:"<<eeprom_data_write.mid(adres*2,8).toUpper();
                    if(has_files)
                    {
                        qDebug()<<"resend can 0102";
                        resend_payload=true;
                        has_to_reset=true;
                    }
                }
                if(!resend_payload){
                    eeprom_data.append(frame.payload().mid(4,4).toHex());
                    adres+=4;
                }
                if(adres<(EE_IO_block.at(run_at)+5*I_max_block.at(run_at)+3*O_max_block.at(run_at))){
                    QString data;
                    data.setNum(adres,16);
                    data=data.rightJustified(4,'0');
                    if(resend_payload){
                        data.prepend("0102");// set
                    }else {
                        data.prepend("0101");// read
                    }
//                    data.append("00000000");
                    data.append(eeprom_data_write.mid(adres*2,8));
                    if(data.size()<15)qDebug()<<"ERROR data.size"<<adres<<data;
//                    data.resize(16,'F');
                    const QByteArray payload =QByteArray::fromHex(data.remove(QLatin1Char(' ')).toLatin1());
//                    qDebug()<<"adres:"<<adres<<"payload:"<<payload;
                    QCanBusFrame frame = QCanBusFrame(frameId, payload);
                    sendFrame(frame);
                } else {
//                    int eeprompointer=0;
//                    qDebug()<<"EEPROM ID      "<<QString::number(eeprompointer,16)<<eeprom_data.mid(eeprompointer,4);//0 kan niet gedeelt worden.
//                    eeprompointer+=4;
//                    qDebug()<<"EEPROM CAN ID  "<<QString::number(eeprompointer/2,16)<<eeprom_data.mid(eeprompointer,2);
//                    eeprompointer+=2;
//                    qDebug()<<"EEPROM DDR     "<<QString::number(eeprompointer/2,16)<<eeprom_data.mid(eeprompointer,8);
//                    eeprompointer+=8;
//                    qDebug()<<"EEPROM Port    "<<QString::number(eeprompointer/2,16)<<eeprom_data.mid(eeprompointer,8);
//                    eeprompointer+=8;
//                    qDebug()<<"EEPROM EXT DDR "<<QString::number(eeprompointer/2,16)<<eeprom_data.mid(eeprompointer,34);
//                    eeprompointer+=34;
//                    qDebug()<<"EEPROM EXT Port"<<QString::number(eeprompointer/2,16)<<eeprom_data.mid(eeprompointer,34);
//                    eeprompointer+=34;
//                    qDebug()<<"EEPROM data";
//                    qDebug()<<QString::number(eeprompointer/2,16)<<eeprom_data.mid(eeprompointer,EE_IO_block.at(run_at)*2-eeprompointer);
//                    eeprompointer+=EE_IO_block.at(run_at)*2-eeprompointer;
//                    qDebug()<<"EEPROM in";
//                    for (int var = 0; var < I_max_block.at(run_at);++var) {
//                        qDebug()<<QString::number(eeprompointer/2,16)<<eeprom_data.mid(eeprompointer,10);
//                        eeprompointer+=10;
//                    }
//                    qDebug()<<"EEPROM uit";
//                    for (int var = 0; var < O_max_block.at(run_at);++var) {
//                        qDebug()<<QString::number(eeprompointer/2,16)<<eeprom_data.mid(eeprompointer,6);
//                        eeprompointer+=6;
//                    }
                    {
                        QFile file("data/out/"+eeprom_data.mid(4,2)+"_in_.txt");
                        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
                        {
                            QTextStream out(&file);
                            out << eeprom_data_write;
                        }
                    }
                    {
                        QFile file("data/out/"+eeprom_data.mid(4,2)+".txt");
                        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
                        {
                            QTextStream out(&file);
                            out << eeprom_data;
                        }
                    }

                    if(has_to_reset){
                        has_to_reset=false;
                        frameId = get_frameId();
                        adres=0;
                        QString data="0404";
                        const QByteArray payload =QByteArray::fromHex(data.remove(QLatin1Char(' ')).toLatin1());
                        QCanBusFrame frame = QCanBusFrame(frameId, payload);
                        sendFrame(frame);
                    }
                    eeprom_data.clear();
                    adres=0;
                    ++run_at;
                    starteeprom();
                }
            } else {
                qDebug()<<frame.payload().mid(2,2).toHex()<<frame.payload().mid(2,2).toUInt();

            }
        }
    }
    return is_Frame;
}

uint MainWindow::get_frameId()
{
    if(protocol_versie.at(run_at)==0x00)return 1024+module_adres.at(run_at);//CAN_Priority_set 0x400
    if(protocol_versie.at(run_at)==0x01)return 1792+module_adres.at(run_at);//CAN_Priority_set 0x700
    if(protocol_versie.at(run_at)==0x02)return 1792+module_adres.at(run_at);//CAN_Priority_set 0x700
    return 0;//ToDo add Warning
}
