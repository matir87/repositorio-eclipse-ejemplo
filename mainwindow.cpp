#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <math.h>       /* pow */
#include <stdlib.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    timerProcesarDato = new QTimer(this);
    timerEnviaComando = new QTimer(this);
    timerPideParametros = new QTimer(this);

    connect(timerProcesarDato, SIGNAL(timeout()), this, SLOT(procesarRespuestaNodo()));
    Gr1 = 0x0810;
    cantidadDeCanales=canalEspecifico=contadorInvalidos=numeroLinea=counterLine=0;
    numeroCanal=1;                          //Lo inicializo a 1 para probar... se sobre escribe.
    ok=true;
    spaceCID=0xF1;
    mecanismoConfigParametros=false;
    mecanismoConfigParametrosUnico=false;
    mecanismoPideParametrosUnico=false;
    mecanismoPideParametros=false;
    flagLog=false;

    for(int i=0;i<30;i++){
        DatosTotal[i]=0;                    //Vector de almacenamiento de datos
        hostcommand[i]=0;
    }
}

MainWindow::~MainWindow(){
    delete ui;
    if(puerto->isOpen())puerto->close();
}


/********************************************  Funciones de CRC16 CITT *************************************************************/

void MainWindow::ResetCRC(){  // Resets the Accumulator - Call before each new CRC value to calculate
    accum = 0;
}

void MainWindow::CrunchCRC (char x){ // Compute CRC using BitbyBit method
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


/******************************** Funciones de Recepción de datos y procesamiento interno ******************************************/

void MainWindow::recibe(){                  //El soft siempre es maestro, manda un comando y espera respuesta
   Data.append(puerto->readAll());          //Guardo toda al info en un array de bytes
   timerProcesarDato->start(10);
}

void MainWindow::procesarRespuestaNodo()    // Decodificación de la información que llega al puerto.
{
    timerProcesarDato->stop();
    Data.append(puerto->readAll());         //a modo de prueba para "re leer el puerto en caso de que haya demora en el envío...

    if(Data.startsWith(0x20)){

        cantDatosCheckCRC = Data.size();
        ui->label_27->setText(" "+QString::number(cantDatosCheckCRC));  //Cantidad de datos leídos en el buffer de entrada (comando completo + crc)
        char *pData =Data.data();
        ResetCRC();

        for (int i=0;i<cantDatosCheckCRC-2;i++){
             CrunchCRC((unsigned char)pData[i]);
        }

        CrunchCRC(0);
        CrunchCRC(0);

        crcComando[1] = (accum & 0xff);             //CRC calculado Baja L
        crcComando[0] = (accum >> 8) & 0xff;        //CRC calculado Alta H

        ui->label_29->setText(" "+ QString::number((unsigned char)crcComando[0])+" - "+ QString::number((unsigned char)crcComando[1] ));  //CRC Calculado

        Head=Data.mid(0,1);
        SCID=Data.mid(1,1);
        virtualCh= Data.mid(2,1);
        byteAmount= Data.mid(3,1);
        FSN= Data.mid(4,1);                         //Acá está el valor que dice si se recibio OK el comando -> 00=OK FF=ERROR...
        ComandID= Data.mid(5,1);

        cantidadDeBytes=0;
        memcpy(&cantidadDeBytes, byteAmount, sizeof(unsigned char));

        if (cantidadDeBytes>6)           //Si hay datos en el campo datos los obtengo en datos.
            datos= Data.mid(6,(cantidadDeBytes-6));   // resto el header sin CRC

        crcDatosRecibH=Data.mid(cantidadDeBytes,1);
        crcDatosRecibL=Data.mid(cantidadDeBytes+1,1);

        unsigned checkCRCH=(unsigned)crcComando[0];
        unsigned checkCRCL=(unsigned)crcComando[1];
        crcRecibidoH=crcRecibidoL=FSNRecibido=0;

        memcpy(&crcRecibidoH, crcDatosRecibH, sizeof(unsigned char));
        memcpy(&crcRecibidoL, crcDatosRecibL, sizeof(unsigned char));

        ui->label_25->setText(" "+QString::number(crcRecibidoH)+" - "+QString::number(crcRecibidoL));  //CRC recibido


        if((checkCRCH ==crcRecibidoH) && (checkCRCL == crcRecibidoL)){  //Check de CRC
            ui->label_28->setText(" ");

            memcpy(&FSNRecibido,FSN, sizeof(unsigned char));

            if (FSNRecibido==0){  //Checkeo de si el comando se recibio correctamente (por parte del equipo MSAP).
                ui->label_28->setText(" Comando recibido correctamente, ID: "+ComandID.toHex());
                ui->textBrowser->append(Head.toHex()+" "+SCID.toHex()+" "+virtualCh.toHex()+" "+byteAmount.toHex()+" "+
                " "+FSN.toHex()+" "+ComandID.toHex()+"  - "+datos.toHex()+"  - "+ crcDatosRecibH.toHex()+ crcDatosRecibL.toHex()); //Solo muestro los datos y su CRC...
                ui->label_26->setText(" "+SCID.toHex());


                if(mecanismoPideParametros==true){  //Mecanismo que vuelve a pedir el proximo canal (parámetros del canal)
                    if(numeroCanal<=(unsigned)cantidadDeCanales){
                        if(flagLog){
                            unsigned char canal= (datos.mid(0,1)).toHex().toUShort(&ok,16);

                            unsigned char pos=datos.mid(1,1).toHex().toUShort(&ok,16);

                            int FactCalibAPE=(datos.mid(2,1).toHex().toShort(&ok,16))<<8;
                                  FactCalibAPE|=(datos.mid(3,1).toHex().toShort(&ok,16))&0xff;

                            unsigned short FactCalibAPD=(datos.mid(4,1).toHex().toUShort(&ok,16))<<8;
                                           FactCalibAPD|=((datos.mid(5,1).toHex().toUShort(&ok,16)))&0xff;
                                          float FactCalibAPEFinal=FactCalibAPE+(FactCalibAPD*0.001);

                            int FactCalibBPE=(datos.mid(6,1).toHex().toShort(&ok,16))<<8;
                                  FactCalibBPE|=(datos.mid(7,1).toHex().toShort(&ok,16))&0xff;

                            unsigned short FactCalibBPD=(datos.mid(8,1).toHex().toUShort(&ok,16))<<8;
                                           FactCalibBPD|=((datos.mid(9,1).toHex().toUShort(&ok,16)))&0xff;
                                           float FactCalibBPEFinal=FactCalibBPE+(FactCalibBPD*0.001);

                            int FactEscAPE=(datos.mid(10,1).toHex().toShort(&ok,16))<<8;
                                  FactEscAPE|=(datos.mid(11,1).toHex().toShort(&ok,16))&0xff;

                            unsigned short FactEscAPD=(datos.mid(12,1).toHex().toUShort(&ok,16))<<8;
                                           FactEscAPD|=((datos.mid(13,1).toHex().toUShort(&ok,16)))&0xff;
                                          float FactEscAPEFinal=FactEscAPE+(FactEscAPD*0.001);

                            int FactEscBPE=(datos.mid(14,1).toHex().toShort(&ok,16))<<8;
                                  FactEscBPE|=(datos.mid(15,1).toHex().toShort(&ok,16))&0xff;

                            unsigned short FactEscBPD=(datos.mid(16,1).toHex().toUShort(&ok,16))<<8;
                                           FactEscBPD|=((datos.mid(17,1).toHex().toUShort(&ok,16)))&0xff;
                                           float FactEscBPEFinal=FactEscBPE+(FactEscBPD*0.001);

                            unsigned char posEsc=datos.mid(18,1).toHex().toShort(&ok,16);

                            QTextStream outParametros(fileLogParametros);   //Contemplo recibir el número de canal en la trama (redundante)
                                        outParametros <<canal<<';'<<pos<<';'<<FactCalibAPEFinal<<';'<<FactCalibBPEFinal<<';'<<FactEscAPEFinal<<';'<<FactEscBPEFinal<<';'<<posEsc<<endl;
                        }

                        numeroCanal++;                                      //Incremento el núemero de canal para que en el próximo envío se mande el pedido del canal siguiente.

                        if(numeroCanal<=(unsigned)cantidadDeCanales){
                            timerPideParametros->singleShot(5,this,SLOT(pideParametros()));
                        }
                        else{
                            mecanismoPideParametros=false;
                            if(fileLogParametros->isOpen())fileLogParametros->close();
                            ui->label_151->setText(" Proceso terminado! ");
                            numeroCanal=1;
                        }
                   }

               }

               if(mecanismoConfigParametros==true)
                   timerEnviaComando->singleShot(5,this,SLOT(lanzaComando()));

            }
            else{
                ui->label_28->setText(" Comando erroneo, retransmitir!, ID: "+ComandID.toHex());

                if((mecanismoPideParametros==true)||(mecanismoPideParametrosUnico==true)){  //CHECKEO QUE SI ESTA PIDIENDO PARAMETROS DE CANAL Y NO RECIBIÓ BIEN EL COMANDO
                    mecanismoPideParametrosUnico=false;
                    ui->label_151->setText(" Petición de parametros FALLIDA. Repetir pedido de canal: "+ QString::number(numeroCanal));
                    ui->textBrowser->append(Head.toHex()+" "+SCID.toHex()+" "+virtualCh.toHex()+" "+byteAmount.toHex()+" "+
                    " "+FSN.toHex()+" "+ComandID.toHex()+"  - "+ crcDatosRecibH.toHex()+ crcDatosRecibL.toHex()); //Solo muestro los datos y su CRC...
                }

                if((mecanismoConfigParametros==true)||mecanismoConfigParametrosUnico==true){ //CHECKEO QUE SI ESTA CONFIGURANDO PARAMETROS DE CANAL Y NO RECIBIÓ BIEN EL COMANDO
                    mecanismoConfigParametrosUnico=false;
                    mecanismoConfigParametros=false;            //Freno el mecanismo de configuración automática
                    ui->label_151->setText(" configuración de parametros FALLIDA. Repetir configuración de canal: "+ QString::number(numeroLinea));
                    ui->textBrowser->append(Head.toHex()+" "+SCID.toHex()+" "+virtualCh.toHex()+" "+byteAmount.toHex()+" "+
                    " "+FSN.toHex()+" "+ComandID.toHex()+"  - "+ crcDatosRecibH.toHex()+ crcDatosRecibL.toHex()); //Solo muestro los datos y su CRC...
                }
            }

       }
       else{
            contadorInvalidos++;
            ui->label_28->setText(" CRC Inválido ");
            ui->label_26->setText(" Inválido");
            ui->label_30->setText(" "+QString::number(contadorInvalidos));

            ui->textBrowser_2->append(Head.toHex()+" "+SCID.toHex()+" "+virtualCh.toHex()+" "+byteAmount.toHex()+" "+
            " "+FSN.toHex()+" "+ComandID.toHex()+"  - "+datos.toHex()+"  - "+ crcDatosRecibH.toHex()+ crcDatosRecibL.toHex()); //Muestro el frame invalido y su CRC..

            Head=SCID=virtualCh=byteAmount=FSN=ComandID=crcDatosRecibH=crcDatosRecibL=0;    //borro todos los parametros cargados

            if(mecanismoPideParametros==true){
                ui->label_151->setText(" Petición de parametros FALLIDA. Repetir pedido de canal: "+ QString::number(numeroCanal));
            }

            if(mecanismoConfigParametros==true){
                ui->label_151->setText(" configuración de parametros FALLIDA. Repetir configuración de canal: "+ QString::number(numeroLinea));
                //ui->textBrowser->append("Repetir configuración de canal: "+ QString::number(numeroLinea));
            }

       }
       Data.clear();
       crcRecibidoH=crcRecibidoL=cantidadDeBytes=FSNRecibido=0; //borro todos los parametros cargados
    }
    else Data.clear();
}


/********************************************************** Config. General ************************************************************************/


void MainWindow::on_comboBox_activated(const QString &arg1){
    puerto= new QSerialPort(this);
    puerto->setPortName(arg1);
    puerto->setBaudRate(QSerialPort::Baud115200);
    puerto->setDataBits(QSerialPort::Data8);
    puerto->setParity(QSerialPort::EvenParity);
    puerto->setStopBits(QSerialPort::OneStop);
    puerto->setFlowControl(QSerialPort::NoFlowControl);

    flagPort=puerto->open(QIODevice::ReadWrite);
    ui->label_3->setText(" Puerto Seleccionado, no abierto ");
}


void MainWindow::on_pushButton_55_clicked(){  //Abrir Puerto
    if((connect(puerto, SIGNAL(readyRead()),SLOT(recibe()))&&(flagPort==true))){
        ui->label_3->setText(" Puerto Abierto ");
        puerto->clear();
    }else{
        ui->label_3->setText(" ERROR en Puerto, seleccione otro puerto.");
    }
}


void MainWindow::on_comboBox_2_activated(const QString &arg1){
    if(puerto->isOpen()==true){
        puerto->setBaudRate(arg1.toInt());
        int baudrate=arg1.toInt();
        ui->label_3->setText(" Puerto Configurado, BaudRate: "+ QString::number(baudrate));
        puerto->clear();
    }
    else ui->label_3->setText(" Error, debe abrir puerto primero.");
}


void MainWindow::on_pushButton_5_clicked(){     //Cerrar Puerto
    if(flagPort==true){
        puerto->clear();
        puerto->close();
        ui->label_3->setText(" Puerto cerrado");
        flagPort=false;
    }else ui->label_3->setText(" Puerto ya cerrado");
}


void MainWindow::on_pushButton_clicked(){       //Apertura de archivo de parametros
    counterLine=0;
    file = QFileDialog::getOpenFileName(this,tr("Abrir Archivo"), "", tr("CSV Files (*.csv)"));

    if (file.isEmpty())
        ui->label_9->setText(" Archivo no abierto");
    else ui->label_9->setText(file);

    QFile fileParametros(file);
    if (!fileParametros.open(QIODevice::ReadOnly)) {
        ui->label_151->setText("Error abriendo para conteo de lineas");
    }

     if(!fileParametros.atEnd()){
         QTextStream in(&fileParametros);

         while(!in.atEnd()) {
             line = in.readLine();
             counterLine++;
         }
    }

     ui->label_151->setText(" Conteo de lineas de valores: "+ QString::number(counterLine));

}

/********************************************************** Operación ************************************************************************/

void MainWindow::on_comboBox_3_activated(int index){     //Select SPACE Craft ID (equipo)
    if(index==0)
        spaceCID=0xF1;
    if(index==1)
        spaceCID=0xF0;
}


void MainWindow::on_pushButton_2_clicked(){            //Comando Modo Ingeniería
    comandoID=0x88;//0x88;
    comSend(6);
    ui->label_151->setText(" Modo Ingeniería");
}


void MainWindow::on_pushButton_3_clicked(){            //Comando Modo Vuelo
    comandoID=0x99;
    comSend(6);
    ui->label_151->setText(" Modo Vuelo");
}

void MainWindow::on_pushButton_4_clicked(){             //lanza comando: Todos los parametros de todos los canales
    mecanismoConfigParametros=true;
    numeroLinea=1;
    timerEnviaComando->singleShot(5,this,SLOT(lanzaComando()));

}

void MainWindow::on_pushButton_15_clicked()   //lanza comando: Configura parametros por Canal
{
    mecanismoConfigParametrosUnico=true;
    numeroLinea++;
    lanzaComando();

}

void MainWindow::on_pushButton_10_clicked() //lanza comando: Configura parametros Canal específico (según número de linea)
{
    mecanismoConfigParametrosUnico=true;
    lanzaComando();
}



void MainWindow::on_pushButton_6_clicked()              //Hace pedido de todos los parametros de todos los canales
{
    mecanismoPideParametros=true;
    numeroCanal=1;
    timerPideParametros->singleShot(5,this,SLOT(pideParametros()));
}

void MainWindow::lanzaComando(){

    QFile fileParametros(file);
    if (!fileParametros.open(QIODevice::ReadOnly)) {
        ui->label_151->setText("Error abriendo [lanzaComando()].csv");
    }

     if(!fileParametros.atEnd()){
         QTextStream in(&fileParametros);

     line = in.readLine();                       //Saco la primer linea  tiene títulos

    if(numeroLinea<=counterLine-1){             //Counter Line tiene la cantidad de lineas de valores del archivo...

        for(unsigned i=0;i<numeroLinea;i++)
            line = in.readLine();                   //leo n lineas, siendo la ultima la que quiero.

         ui->label_151->setText(" Linea: " + QString::number(numeroLinea));
         QStringList parametros= line.split(';');
         parametros.removeFirst();                       //Nombre del sensor (no lo tengo que mandar)
         /***************************************************************************************/

         hostcommand[6]=parametros.first().toUShort();   //Número de Canal  //.toUShort(&ok,16);   //Número de Canal
         parametros.removeFirst();
         /***************************************************************************************/

         hostcommand[7]=parametros.first().toUShort();   // Posicion
         parametros.removeFirst();
         /***************************************************************************************/

         floatParam=parametros.first().split('.');       //guardo parte entera y parte decimal en 4 bytes (factCalibA)

         hostcommand[8]=(floatParam.first().toShort())>>8;
         hostcommand[9]=floatParam.first().toShort();
         floatParam.removeFirst();

         hostcommand[10]=((floatParam.first().toUShort())>>8);
         hostcommand[11]=((floatParam.first().toUShort()));
         floatParam.removeFirst();
         parametros.removeFirst();
         /***************************************************************************************/

         floatParam=parametros.first().split('.');       //guardo parte entera y parte decimal en 4 bytes (factCalibB)

         hostcommand[12]=(floatParam.first().toShort())>>8;
         hostcommand[13]=floatParam.first().toShort();
         floatParam.removeFirst();

         hostcommand[14]=((floatParam.first().toUShort())>>8);
         hostcommand[15]=((floatParam.first().toUShort()));
         floatParam.removeFirst();
         parametros.removeFirst();
         /***************************************************************************************/

         floatParam=parametros.first().split('.');       //guardo parte entera y parte decimal en 4 bytes (factEscalA)

         hostcommand[16]=(floatParam.first().toShort())>>8;
         hostcommand[17]=floatParam.first().toShort();
         floatParam.removeFirst();

         hostcommand[18]=((floatParam.first().toUShort())>>8);
         hostcommand[19]=((floatParam.first().toUShort()));
         floatParam.removeFirst();
         parametros.removeFirst();
         /***************************************************************************************/

         floatParam=parametros.first().split('.');       //guardo parte entera y parte decimal en 4 bytes (factEscalB)

         hostcommand[20]=(floatParam.first().toShort())>>8;
         hostcommand[21]=floatParam.first().toShort();
         floatParam.removeFirst();

         hostcommand[22]=((floatParam.first().toUShort())>>8);
         hostcommand[23]=((floatParam.first().toUShort()))&0xff;
         floatParam.removeFirst();
         parametros.removeFirst();                       //borro el Factor Escalacion B
         /***************************************************************************************/

         hostcommand[24]=parametros.first().toShort();      //posEscTrama

         comandoID=0x66;//0x66;  //poner ID que envía todo...
         comSend(25);
         if(mecanismoConfigParametros)numeroLinea++;

    }
     else{
         ui->label_151->setText(" Fin envío de parametros");
         mecanismoConfigParametros=false;
         numeroLinea=0;
         fileParametros.close();
    }
  }
}

void MainWindow::pideParametros(){

    comandoID=0x31;
    ui->label_151->setText(" Pedido de parametros "+QString::number(numeroCanal));
    hostcommand[6]=numeroCanal;                         //Número de canal
    comSend(7);
}

void MainWindow::on_pushButton_9_clicked()   //pide parametros canal especifico
{   mecanismoPideParametrosUnico=true;
    comandoID=0x31;
    ui->label_151->setText(" Pedido de parametros "+QString::number(canalEspecifico));
    hostcommand[6]=canalEspecifico;                         //Número de canal
    comSend(7);
}


void MainWindow::comSend(unsigned short longTotByte){

    ui->label_5->setText("");
    if(puerto->isOpen()==true){
        hostcommand[0]=0x20;                            // Header ID
        hostcommand[1]=spaceCID;
        hostcommand[2]=(longTotByte >> 8) & 0xff;       // Longitud del comando completo, medido en palabras words de 16 bits
        hostcommand[3]=longTotByte & 0xff;
        hostcommand[4]=0x00;
        hostcommand[5]=comandoID;

        ResetCRC();
        for (int i=0;i<(int)(longTotByte);i++) {
          CrunchCRC(hostcommand[i]);
        }

        CrunchCRC(0);
        CrunchCRC(0);
        crcComando[1] = accum & 0xff;
        crcComando[0] = (accum >> 8) & 0xff;

        puerto->write((const char*)hostcommand,(longTotByte));
        puerto->write((const char*)crcComando,2);

        QByteArray cabeceraDatos=QByteArray((const char*)hostcommand,longTotByte);
        QByteArray crc=QByteArray((const char*)crcComando,2);

        ui->label_5->setText(cabeceraDatos.toHex()+ "  " + crc.toHex());

    }else ui->label_3->setText(" ERROR en Puerto,No se envían comandos, verifique configuración de puerto");

    Data.clear();
}

/********************************************************************************************************************************************************/

void MainWindow::on_pushButton_8_clicked()   //Borrador de Respuestas
{
    ui->label_25->setText(" "); ui->label_26->setText(" "); ui->label_27->setText(" "); ui->label_28->setText(" ");ui->label_29->setText(" ");ui->label_30->setText(" ");
    ui->textBrowser->clear();
    ui->textBrowser_2->clear();
    contadorInvalidos=0;
}



void MainWindow::on_checkBox_clicked(bool checked)      //Checkbox para la creacion de los logs de los parametros.
{
    if(checked==true){
        QString path =QFileDialog::getExistingDirectory(this,tr("Seleccionar directorio"),"");

        filenameLog = path+"/";
        ui->label_151->setText(" "+filenameLog);

        QDateTime dateTime = QDateTime::currentDateTime();
        QString fecha=dateTime.toString("dd_MM_yyyy_hh_mm_ss");

        fileLogParametros =new QFile(filenameLog+"Parametros_por_canal_"+fecha+".csv");

        //open in WriteOnly and Text mode
        if(!fileLogParametros->open(QFile::WriteOnly | QFile::Text)){
            ui->label_151->setText( " No se puede abrir el archivo, error...");
            return;
        }
        QTextStream outParametros(fileLogParametros);
                    outParametros <<"NUM. CANAL; "<<"POSICION;"<<"FACT. CALIBRACION A;"<<"FACT. CALIBRACION B;"<<"FACT. ESCALADO A;"<<"FACT. ESCALADO B;"<<"POS. ESCALADO EN TRAMA;"<<endl;
        flagLog=true;

    }
    else{
        if(flagLog==true){
           ui->label_151->setText(" Logueo terminado!");
           flagLog=false;
           if(fileLogParametros->isOpen()){
                fileLogParametros->flush();
                fileLogParametros->close();
           }
        }
    }
}

void MainWindow::on_spinBox_valueChanged(int arg1)  //cantidad de canales en equipo.
{
    cantidadDeCanales=arg1;
    ui->label_151->setText(" Cantidad de Canales: "+QString::number(cantidadDeCanales));
    ui->lineEdit->clear();
}

void MainWindow::on_lineEdit_2_textChanged(const QString &arg1)  //Seleccionar un canal particular para configurar (elegir linea del .csv)
{
    numeroLinea=arg1.toInt();
    if(numeroLinea<counterLine){
        ui->label_151->setText(" Linea N°: "+QString::number(numeroLinea));
        ui->spinBox->clear();
    }else{
        ui->label_151->setText(" N° de Canal fuera de rango, mayor al máximo de la tabla de configuración, Ingrese nuevo valor ");
    }
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1)  //un solo canal especifico
{
    canalEspecifico=arg1.toInt();
    ui->label_151->setText(" Canal N°: "+QString::number(canalEspecifico));
    ui->spinBox->clear();
}

void MainWindow::on_pushButton_7_clicked()      //Mecanismo de mostrar tabla de valores cargados en el msap (logs).
{
    model = new QStandardItemModel(this);
    ui->tableView->setModel(model);

    if (fileLogParametros->open(QIODevice::ReadOnly)) {
        QString data = fileLogParametros->readAll();
        data.remove( QRegExp("\r") ); //remove all ocurrences of CR (Carriage Return)
        QString temp;
        QChar character;
        QTextStream textStream(&data);
        while (!textStream.atEnd()) {
            textStream >> character;
            if (character == ';') {
                checkString(temp, character);
            } else if (character == '\n') {
                checkString(temp, character);
            } else if (textStream.atEnd()) {
                temp.append(character);
                checkString(temp);
            } else {
                temp.append(character);
            }
        }
    }
    fileLogParametros->close();

}

void MainWindow::checkString(QString &temp, QChar character)    //ARMA LOS DATOS PARA EL VISUALIZADOR
{
    if(temp.count("\"")%2 == 0) {
        if (temp.startsWith( QChar('\"')) && temp.endsWith( QChar('\"') ) ) {
             temp.remove( QRegExp("^\"") );
             temp.remove( QRegExp("\"$") );
        }
        temp.replace("\"\"", "\"");
        QStandardItem *item = new QStandardItem(temp);
        standardItemList.append(item);
        if (character != QChar(';')) {
            model->appendRow(standardItemList);
            standardItemList.clear();
        }
        temp.clear();
    } else {
        temp.append(character);
    }
}
