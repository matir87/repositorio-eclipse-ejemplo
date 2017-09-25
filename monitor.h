#ifndef MONITOR_H
#define MONITOR_H

#include <QThread>
#include <QString>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <QtCore/QCoreApplication>
#include <QtSerialPort/QtSerialPort>
#include <QTime>
#include <QMutex>

class Monitor : public QThread
{
    Q_OBJECT
    public:
        explicit Monitor(QObject *parent=0);
        void run();
        void comMonitor(unsigned char index,unsigned char offset);
        void ResetCRC();
        void CrunchCRC (char x);
        void setMonitorConfig(QSerialPort *puerto,QMutex *mutex, unsigned* comandID, unsigned* comandTipo,unsigned* DELAY_LATENCIA);
        void SetcomandoId(unsigned id, unsigned tipo);

    private slots:
            void CuentaEncoder();
            void SensoresHall();

    private:

        QSerialPort *puertoM;
        unsigned char hostcommandM[8];
        int frecMonitor;
        QMutex *mutexMon;
        QTimer *timerRun;
        QTimer *timerRun2;
        unsigned DELAY;


        /******/

        unsigned secuencia;

        unsigned int accum;
        unsigned int Gr1;
        QTimer *timerProcesarDato;
        QTimer *timerEsperaDato;
        QTimer *timerLanzaComandos;
        Monitor* threadMonitor;
        QMutex* mutex;

        QByteArray Data;
        bool flagData;
        unsigned char ComandoRecibido[15];  //Array de la cabecera del comando recibido
        unsigned char hostcommand[8];  //Array del comando a enviar - VER TAMAÑO FINAL
        unsigned DELAY_LATENCIAMON;
        unsigned char datavector[10]; //separados de a 8 bytes no words!
        unsigned char crcDatosArray[2];
        unsigned char cmdindex;
        unsigned char cmdoffset;
        unsigned char cmdwordsnumber;
        unsigned char ctrlByte;

        unsigned* comandIDMonitor;
        unsigned* comandTipoMonitor;


        int valorI;
        unsigned valorU;
        short valorS;
        unsigned short valorUS;
        int frecMonitoreo;


        QByteArray sof;
        QByteArray address;
        QByteArray controlByte;
        QByteArray status1;
        QByteArray status2;
        QByteArray dataWords;
        QByteArray crcHead;
        QByteArray datos;
        QByteArray crcDatos;


};

#endif // MONITOR_H
