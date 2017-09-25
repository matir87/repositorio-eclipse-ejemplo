#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <QtCore/QCoreApplication>
#include <QtSerialPort/QtSerialPort>
#include <QInputDialog>
#include <QMessageBox>
#include <QTime>
#include <QThread>
#include <QTextStream>
#include <QDateTime>
#include <QFileDialog>
#include <QFile.h>
#include <QStandardItemModel>

namespace Ui {class MainWindow;}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void comSend(unsigned short longTotByte);
    void ResetCRC();
    void CrunchCRC(char x);
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    void checkString(QString &temp, QChar character = 0);

private slots:
    void recibe();
    void procesarRespuestaNodo();
    void lanzaComando();
    void pideParametros();

    void on_comboBox_activated(const QString &arg1);
    void on_comboBox_2_activated(const QString &arg1);
    void on_comboBox_3_activated(int index);

    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_8_clicked();
    void on_pushButton_55_clicked();
    void on_pushButton_6_clicked();
    void on_pushButton_7_clicked();
    void on_pushButton_9_clicked();
    void on_pushButton_10_clicked();
    void on_pushButton_15_clicked();

    void on_checkBox_clicked(bool checked);
    void on_spinBox_valueChanged(int arg1);
    void on_lineEdit_textChanged(const QString &arg1);
    void on_lineEdit_2_textChanged(const QString &arg1);



private:

    Ui::MainWindow *ui;
    QSerialPort *puerto; //QSerialPort *puerto, unsigned char hostcommand[8];
    FILE *archivo;
    QString line;
    QFile *fileLogParametros;
    char linea[100];                            //100 lineas de datos (una por cada sensor (sobran))
    int cantDatosLinea;
    int cantDatosCheckCRC;
    int contadorInvalidos;
    unsigned char DatosTotal[30];
    unsigned crcRecibidoH;
    unsigned crcRecibidoL;
    unsigned FSNRecibido;
    unsigned numeroCanal;
    unsigned cantidadDeBytes;
    unsigned counterLine;       //Contador de leídos
    unsigned numeroLinea;       //Contador de envíos
    char DatosCheckCRC[30];
    char *token;
    int indice;
    int cantidadDeCanales;
    int canalEspecifico;
    bool flagPort;
    bool flagLog;
    bool ok;
    bool mecanismoPideParametros;
    bool mecanismoPideParametrosUnico;
    bool mecanismoConfigParametros;
    bool mecanismoConfigParametrosUnico;
    unsigned int accum;
    unsigned int Gr1;
    int millisecondsWait;
    QString file;
    QString filenameLog;
    QMessageBox msgBox;
    QTimer *timerProcesarDato;
    QTimer *timerEnviaComando;
    QTimer *timerPideParametros;

    QByteArray Data;
    char hostcommand[30];  //Array del comando a enviar - VER TAMAÑO FINAL
    QStringList floatParam;    //alocador temporal de parte de float.

    unsigned char crcComando[2];
    unsigned char cmdwordsnumber;
    unsigned char ctrlByte;
    unsigned char spaceCID;
    unsigned char comandoID;
    char* datosRtaNodo[];
    unsigned comandID;
    unsigned comandTipo;
    unsigned lanzador;

    QByteArray Head;
    QByteArray SCID;
    QByteArray virtualCh;
    QByteArray byteAmount;
    QByteArray FSN;
    QByteArray ComandID;
    QByteArray datos;
    QByteArray crcDatosRecibH;
    QByteArray crcDatosRecibL;

    QStandardItemModel *model;
    QList<QStandardItem*> standardItemList;
};

#endif // MAINWINDOW_H
