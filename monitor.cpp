#include "monitor.h"
#include <QDebug>

#define INT     1
#define UINT    2
#define SHORT   3
#define USHORT  4

Monitor::Monitor(QObject *parent):
    QThread(parent)
{
    timerRun = new QTimer;
    timerRun2 = new QTimer;
}


void Monitor::setMonitorConfig(QSerialPort *puerto,QMutex *mutex, unsigned* comandID, unsigned* comandTipo,unsigned* DELAY_LATENCIA)
{
    puertoM=puerto;
    mutexMon=mutex;
    comandIDMonitor=comandID;
    comandTipoMonitor=comandTipo;
    DELAY=*DELAY_LATENCIA;
}


void Monitor::run()
{
    connect(timerRun, SIGNAL(timeout()), this, SLOT(CuentaEncoder()));
    connect(timerRun2, SIGNAL(timeout()), this, SLOT(SensoresHall()));

    timerRun->start(1125);
    timerRun2->start(1950);

    exec();
}


void Monitor::CuentaEncoder() {
  comMonitor(0x0E,0x00);
  SetcomandoId(1,INT);
}

void Monitor::SensoresHall() {
      comMonitor(0x0E,0x06);
      SetcomandoId(2,USHORT);
}


void Monitor::SetcomandoId(unsigned id, unsigned tipo){
    *comandIDMonitor=id;
    *comandTipoMonitor=tipo;
}

void Monitor::comMonitor(unsigned char index,unsigned char offset) {  //Metodo de consulta de comandos....

    ResetCRC();
    hostcommand[0]=0xA5;              // Header
    hostcommand[1]=0x3F;              // Address  63 decimal
    hostcommand[2]=0x01;              // control byte -> no lleva datos...
    hostcommand[3]=index;             // index
    hostcommand[4]=offset;            // offset
    hostcommand[5]=0;                 // número de palabras  (2 bytes)

    for (int i=0; i<=5; i++)
        CrunchCRC(hostcommand[i]);

    CrunchCRC(0);
    CrunchCRC(0);
    hostcommand[7] = accum & 0xff;
    hostcommand[6] = (accum >> 8) & 0xff;

    puertoM->write((const char*)hostcommand,8);
}



/********************************************  Funciones de CRC16 CITT *************************************************************/

void Monitor::ResetCRC()  // Resets the Accumulator - Call before each new CRC value to calculate
{
    accum = 0;
}

void Monitor::CrunchCRC (char x) // Compute CRC using BitbyBit method
{
    int i;
    for(int k=0;k<8;k++){
        i = (x >> 7) & 1;
        if (accum & 0x8000)
            accum = ((accum ^ Gr1) << 1) + (i ^ 1);
        else
            accum = (accum << 1) + i;

        accum &= 0x0ffff;
        x <<= 1;
    }
}
/***************************************************************************************************************************************/
