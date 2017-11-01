#include "tcontrollerrpi.h"
#include "ui_tcontrollerrpi.h"
#include "utility.h"
#include "clientlistdialog.h"
#include "mcp4725.h"


#include <QNetworkInterface>
#include <QUdpSocket>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QCloseEvent>
#include <math.h>
#include <QDebug>

#define CONNECTION_TIME       3000// Not to be set too low for coping with slow networks
#define NETWORK_CHECK_TIME    3000

#define DISCOVERY_PORT      45453
#define SERVER_SOCKET_PORT  45454


TControllerRPi::TControllerRPi(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TControllerRPi)
  , percentage(0.0)
  , commandVoltage(percentage/330.0)
  , logFile(Q_NULLPTR)
  , pServerSocket(Q_NULLPTR)
  , pPanelServer(Q_NULLPTR)
  , pClientListDialog(new ClientListDialog(this))
  , discoveryPort(DISCOVERY_PORT)
  , discoveryAddress(QHostAddress("224.0.0.1"))
  , serverPort(SERVER_SOCKET_PORT)
{
    ui->setupUi(this);

    ui->powerPercentageEdit->setToolTip("Enter a Value between 0.0 and 100.0");
    sString = QString("%1").arg(percentage, 0, 'f', 1);
    ui->powerPercentageEdit->setText(sString);
    ui->applyButton->hide();

    sNormalStyle = ui->powerPercentageEdit->styleSheet();
    sErrorStyle  = "QLineEdit { background: rgb(255, 0, 0); selection-background-color: rgb(255, 255, 0); }";

    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    // create docks, toolbars, etc...
    restoreState(settings.value("mainWindowState").toByteArray());

    QString sFunctionName = QString(" TControllerRPi::TControllerRPi ");
    Q_UNUSED(sFunctionName)

    QTime time(QTime::currentTime());
    qsrand(time.msecsSinceStartOfDay());

    // Prepare a logfile to log messages
    QString sBaseDir;
    sBaseDir = QDir::homePath();
    if(!sBaseDir.endsWith(QString("/"))) sBaseDir+= QString("/");
    logFileName = QString("%1TController.txt").arg(sBaseDir);
    prepareLogFile();

    // Start listening for connections on all available interfaces
    prepareDiscovery();

    // Prepare the server for Remote Control
    prepareServer();

//    connect(&updateTimer, SIGNAL(timeout()),
//            this, SLOT(onTimeToUpdate()));
//    updateTimer.start(100);

    // Start the DAQ Tasks
    pDAC = new MCP4725(QString("/dev/i2c-1"), 0x60);
    startDAQ();
}


TControllerRPi::~TControllerRPi() {
    delete ui;
}


// Start listening for connection requests on all available interfaces
void
TControllerRPi::prepareDiscovery() {
    QString sFunctionName = QString(" TControllerRPi::prepareDiscovery ");
    sIpAddresses = QStringList();
    QList<QNetworkInterface> interfaceList = QNetworkInterface::allInterfaces();
    for(int i=0; i<interfaceList.count(); i++)
    {
        QNetworkInterface interface = interfaceList.at(i);
        if(interface.flags().testFlag(QNetworkInterface::IsUp) &&
           interface.flags().testFlag(QNetworkInterface::IsRunning) &&
           interface.flags().testFlag(QNetworkInterface::CanMulticast) &&
          !interface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            QList<QNetworkAddressEntry> list = interface.addressEntries();
            for(int j=0; j<list.count(); j++)
            {
                QUdpSocket* pDiscoverySocket = new QUdpSocket(this);
                if(list[j].ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    if(pDiscoverySocket->bind(QHostAddress::AnyIPv4, discoveryPort, QUdpSocket::ShareAddress)) {
                        pDiscoverySocket->joinMulticastGroup(discoveryAddress);
                        sIpAddresses.append(list[j].ip().toString());
                        discoverySocketArray.append(pDiscoverySocket);
                        connect(pDiscoverySocket, SIGNAL(readyRead()),
                                this, SLOT(onProcessConnectionRequest()));
#ifdef LOG_VERBOSE
                        logMessage(logFile,
                                   sFunctionName,
                                   QString("Listening for connections at address: %1 port:%2")
                                   .arg(discoveryAddress.toString())
                                   .arg(discoveryPort));
#endif
                    }
                    else {
                        logMessage(logFile,
                                   sFunctionName,
                                   QString("Unable to bound %1")
                                   .arg(discoveryAddress.toString()));
                    }
                }
            }// for(int j=0; j<list.count(); j++)
        }
    }// for(int i=0; i<interfaceList.count(); i++)
}


// Prepare the server for Remote Control
int
TControllerRPi::prepareServer() {
  QString sFunctionName = " TControllerRPi::prepareServer ";
  Q_UNUSED(sFunctionName)

  pPanelServer = new QWebSocketServer(QStringLiteral("Server"),
                                      QWebSocketServer::NonSecureMode,
                                      this);
  connect(pPanelServer, SIGNAL(newConnection()),
          this, SLOT(onNewPanelServerConnection()));
  connect(pPanelServer, SIGNAL(serverError(QWebSocketProtocol::CloseCode)),
          this, SLOT(onPanelServerError(QWebSocketProtocol::CloseCode)));
  if (!pPanelServer->listen(QHostAddress::Any, serverPort)) {
    logMessage(logFile,
               sFunctionName,
               QString("Impossibile ascoltare la porta %2 !")
               .arg(serverPort));
    return -7;
  }
  logMessage(logFile,
             sFunctionName,
             QString("listening on port:%2")
             .arg(serverPort));

  return 0;
}


void
TControllerRPi::onPanelServerError(QWebSocketProtocol::CloseCode closeCode){
  QString sFunctionName = " TControllerRPi::onPanelServerError ";
  logMessage(logFile,
             sFunctionName,
             QString("%2 Close code: %3")
             .arg(pPanelServer->serverName())
             .arg(closeCode));
}


//void
//TControllerRPi::onTimeToUpdate() {
//  if(!connectionList.isEmpty()) {
//    sString = QString("<readPercent>%1</readPercent>").arg(ui->powerPercentageReadEdit->text());
//    sendToAll(sString);
//  }
//}


void
TControllerRPi::onNewPanelServerConnection() {
    QString sFunctionName = " TControllerRPi::onNewPanelServerConnection ";
    Q_UNUSED(sFunctionName)

    QWebSocket *pClient = pPanelServer->nextPendingConnection();
    logMessage(logFile,
               sFunctionName,
               QString("%1 - Client %2 connected")
               .arg(pPanelServer->serverName())
               .arg(pClient->peerAddress().toString()));

    QHostAddress address = pClient->peerAddress();
    QString sAddress = address.toString();
    connect(pClient, SIGNAL(textMessageReceived(QString)),
            this, SLOT(onProcessTextMessage(QString)));
    connect(pClient, SIGNAL(binaryMessageReceived(QByteArray)),
            this, SLOT(onProcessBinaryMessage(QByteArray)));
    connect(pClient, SIGNAL(disconnected()),
            this, SLOT(onClientDisconnected()));
    RemoveClient(address);

    connection newConnection;
    newConnection.pClientSocket = pClient;
    newConnection.clientAddress = address;
    connectionList.append(newConnection);
    pClientListDialog->addItem(sAddress);
    logMessage(logFile,
               sFunctionName,
               QString("Client connected: %1")
               .arg(sAddress));
    ui->statusBar->showMessage(QString("Client connected: %1").arg(sAddress));
}


void
TControllerRPi::closeServer() {
    if(pPanelServer)
      pPanelServer->close();
    pPanelServer = Q_NULLPTR;
}


// We prepare a file to write the session log
bool
TControllerRPi::prepareLogFile() {
#ifdef LOG_MESG
    QString sFunctionName = " TControllerRPi::PrepareLogFile ";
    Q_UNUSED(sFunctionName)

    QFileInfo checkFile(logFileName);
    if(checkFile.exists() && checkFile.isFile()) {
        QDir renamed;
        renamed.remove(logFileName+QString(".bkp"));
        renamed.rename(logFileName, logFileName+QString(".bkp"));
    }
    logFile = new QFile(logFileName);
    if (!logFile->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, sFunctionName,
                                 tr("Impossibile aprire il file %1: %2.")
                                 .arg(logFileName).arg(logFile->errorString()));
        delete logFile;
        logFile = Q_NULLPTR;
    }
#endif
    return true;
}


void
TControllerRPi::startDAQ() {
    QString sFunctionName = " TControllerRPi::startDAQ ";
    Q_UNUSED(sFunctionName)
    if(pDAC->Initialize()) {
#ifdef LOG_MESG
        logMessage(logFile,
                   sFunctionName,
                   QString("Unable to initialize the DA Converter"));
#endif
        exit(-1);
    }
    DAQWrite(0.0);
}


// returns true if the network is available
bool
TControllerRPi::isConnectedToNetwork() {
    QString sFunctionName = " TControllerRPi::isConnectedToNetwork ";
    Q_UNUSED(sFunctionName)
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    bool result = false;

    for(int i=0; i<ifaces.count(); i++) {
        QNetworkInterface iface = ifaces.at(i);
        if(iface.flags().testFlag(QNetworkInterface::IsUp) &&
           iface.flags().testFlag(QNetworkInterface::IsRunning) &&
           iface.flags().testFlag(QNetworkInterface::CanMulticast) &&
          !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            for(int j=0; j<iface.addressEntries().count(); j++) {
                // we have an interface that is up, and has an ip address
                // therefore the link is present
                if(result == false)
                    result = true;
            }
        }
    }
#ifdef LOG_VERBOSE
    logMessage(logFile,
               sFunctionName,
               result ? QString("true") : QString("false"));
#endif
    return result;
}


// Process connection requests
void
TControllerRPi::onProcessConnectionRequest() {
  QString sFunctionName = " TControllerRPi::onProcessConnectionRequest ";
  QByteArray datagram, request;
  QString sToken;
  QUdpSocket* pDiscoverySocket = qobject_cast<QUdpSocket*>(sender());
  QString sNoData = QString("NoData");
  QString sMessage;
  Q_UNUSED(sMessage)
  QHostAddress hostAddress;
  quint16 port;

  while(pDiscoverySocket->hasPendingDatagrams()) {
    datagram.resize(pDiscoverySocket->pendingDatagramSize());
    pDiscoverySocket->readDatagram(datagram.data(), datagram.size(), &hostAddress, &port);
    request.append(datagram.data());
  }
  sToken = XML_Parse(request.data(), "getServer");
  if(sToken != sNoData) {
    sendAcceptConnection(pDiscoverySocket, hostAddress, port);
    logMessage(logFile,
               sFunctionName,
               QString("Connection request from: %1 at Address %2:%3")
               .arg(sToken)
               .arg(hostAddress.toString())
               .arg(port));
    RemoveClient(hostAddress);
#ifdef LOG_VERBOSE
    logMessage(logFile,
               sFunctionName,
               QString("Sent: %1")
               .arg(sMessage));
#endif
  }
}


int
TControllerRPi::sendAcceptConnection(QUdpSocket* pDiscoverySocket, QHostAddress hostAddress, quint16 port) {
    QString sFunctionName = " TControllerRPi::sendAcceptConnection ";
    Q_UNUSED(sFunctionName)
    QString sString = QString("%1").arg(sIpAddresses.at(0));
    for(int i=1; i<sIpAddresses.count(); i++) {
      sString += QString(";%1").arg(sIpAddresses.at(i));
    }
    QString sMessage = "<serverIP>" + sString + "</serverIP>";
    QByteArray datagram = sMessage.toUtf8();
    if(!pDiscoverySocket->isValid()) {
      logMessage(logFile,
                 sFunctionName,
                 QString("Discovery Socket Invalid !"));
      return -1;
    }
    qint64 bytesWritten = pDiscoverySocket->writeDatagram(datagram.data(), datagram.size(), hostAddress, port);
    Q_UNUSED(bytesWritten)
    if(bytesWritten != datagram.size()) {
      logMessage(logFile,
                 sFunctionName,
                 QString("Unable to send data !"));
    }
    return 0;
}


void
TControllerRPi::RemoveClient(QHostAddress hAddress) {
    QString sFunctionName = " TControllerRPi::RemoveClient ";
    QString sFound = tr(" Not present");
    Q_UNUSED(sFunctionName)
    Q_UNUSED(sFound)

    QWebSocket *pClientToClose = NULL;
    pClientListDialog->clear();

    for(int i=connectionList.count()-1; i>=0; i--) {
        if((connectionList.at(i).clientAddress.toIPv4Address() == hAddress.toIPv4Address()))
        {
            pClientToClose = connectionList.at(i).pClientSocket;
            disconnect(pClientToClose, 0, 0, 0); // No more events from this socket
            pClientToClose->close(QWebSocketProtocol::CloseCodeAbnormalDisconnection, tr("Timeout in connection"));
            connectionList.removeAt(i);
#ifdef LOG_VERBOSE
            sFound = " Removed !";
            logMessage(logFile,
                       sFunctionName,
                       QString("%1 %2")
                       .arg(hAddress.toString())
                       .arg(sFound));
#endif
        } else {
            pClientListDialog->addItem(connectionList.at(i).clientAddress.toString());
        }
    }
    if(pClientToClose != NULL) {
        pClientToClose->abort();
        pClientToClose->deleteLater();
    }
}


void
TControllerRPi::onClientDisconnected() {
    QString sFunctionName = " TControllerRPi::onClientDisconnected ";
    QWebSocket* pClient = qobject_cast<QWebSocket *>(sender());
    QString sDiconnectedAddress = pClient->peerAddress().toString();
    logMessage(logFile,
               sFunctionName,
               QString("%1 disconnected because %2. Close code: %3")
               .arg(sDiconnectedAddress)
               .arg(pClient->closeReason())
               .arg(pClient->closeCode()));
    RemoveClient(pClient->peerAddress());
    ui->statusBar->clearMessage();
}


void
TControllerRPi::onProcessTextMessage(QString sMessage) {
    QString sFunctionName = " TControllerRPi::onProcessTextMessage ";
    Q_UNUSED(sFunctionName)
    QString sToken;
    QString sNoData = QString("NoData");

    sToken = XML_Parse(sMessage, "getStatus");
    if(sToken != sNoData) {
        QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
        sendToOne(pClient, FormatStatusMsg());
    }// getStatus

    sToken = XML_Parse(sMessage, "setPercent");
    if(sToken != sNoData) {
      ui->powerPercentageEdit->setText(sToken);
      on_applyButton_clicked();
    }// setPercent

}


void
TControllerRPi::onProcessBinaryMessage(QByteArray message) {
  QString sFunctionName = " TControllerRPi::onProcessBinaryMessage ";
  Q_UNUSED(message)
  logMessage(logFile,
             sFunctionName,
             QString("Unexpected binary message received !"));
}


QString
TControllerRPi::FormatStatusMsg() {
    QString sFunctionName = " TControllerRPi::FormatStatusMsg ";
    Q_UNUSED(sFunctionName)
    QString setVal = ui->powerPercentageEdit->text();
//    QString readVal = ui->powerPercentageReadEdit->text();
//    QString sMessage = QString("<setPercent>%1</setPercent><readPercent>%2</readPercent>").arg(setVal).arg(readVal);
    QString sMessage = QString("<setPercent>%1</setPercent>").arg(setVal);
    return sMessage;
}


int
TControllerRPi::sendToAll(QString sMessage) {
  QString sFunctionName = " TControllerRPi::SendToAll ";
  Q_UNUSED(sFunctionName)
#ifdef LOG_VERBOSE
  logMessage(logFile,
             sFunctionName,
             sMessage);
#endif
  for(int i=0; i< connectionList.count(); i++) {
    sendToOne(connectionList.at(i).pClientSocket, sMessage);
  }
  return 0;
}


int
TControllerRPi::sendToOne(QWebSocket* pClient, QString sMessage) {
  QString sFunctionName = " TControllerRPi::SendToOne ";
  if (pClient->isValid()) {
      for(int i=0; i< connectionList.count(); i++) {
         if(connectionList.at(i).clientAddress.toIPv4Address() == pClient->peerAddress().toIPv4Address()) {
              qint64 written = pClient->sendTextMessage(sMessage);
              Q_UNUSED(written)
              if(written != sMessage.length()) {
                  logMessage(logFile,
                             sFunctionName,
                             QString("Error writing %1").arg(sMessage));
              }
#ifdef LOG_VERBOSE
              else {
                  logMessage(logFile,
                             sFunctionName,
                             QString("Sent %1 to: %2")
                             .arg(sMessage)
                             .arg(pClient->peerAddress().toString()));
              }
#endif
              break;
          }
      }
  }
  else {
    logMessage(logFile,
               sFunctionName,
               QString("Client socket is invalid !"));
    RemoveClient(pClient->peerAddress());
  }
  return 0;
}


void
TControllerRPi::closeEvent(QCloseEvent *event) {
  QString sFunctionName = " TControllerRPi::closeEvent ";
  QString sMessage;
  Q_UNUSED(sFunctionName)
  Q_UNUSED(sMessage)
  Q_UNUSED(event)

  // Close all DAQ tasks
  commandVoltage = 0.0;
  DAQWrite(commandVoltage);
  CloseDAQ();

  // Close all the discovery sockets
  for(int i=0; i<discoverySocketArray.count(); i++) {
    disconnect(discoverySocketArray.at(i), 0, 0, 0);
    discoverySocketArray.at(i)->close();
  }
  // Close all the connections
  if(connectionList.count() > 0) {
    for(int i=0; i<connectionList.count(); i++) {
      disconnect(connectionList.at(i).pClientSocket, 0, 0, 0);
      connectionList.at(i).pClientSocket->close(QWebSocketProtocol::CloseCodeNormal, "Server Closed");
    }
    connectionList.clear();
  }
  pPanelServer->close();
  settings.setValue("mainWindowGeometry", saveGeometry());
  settings.setValue("mainWindowState", saveState());
  if(logFile) {
      logFile->flush();
      logFile->close();
      delete logFile;
      logFile = Q_NULLPTR;
  }
}


void
TControllerRPi::CloseDAQ() {
  if(pDAC) delete pDAC;
}


void
TControllerRPi::DAQWrite(double dValue) {
    int16_t iVal = int16_t(4095.0*dValue);
    pDAC->WriteData(iVal);
}


void
TControllerRPi::onExitProgram() {
  QMessageBox::information(this, "onExitProgram", "onExitProgram");
}


void
TControllerRPi::on_powerPercentageEdit_returnPressed() {
  double pValue = ui->powerPercentageEdit->text().toDouble();
  if((pValue < 0.0) || (pValue > 100.0)) {
    ui->powerPercentageEdit->setStyleSheet(sErrorStyle);
    return;
  }
  percentage = pValue;
  sString = QString("%1").arg(percentage, 0, 'f', 1);
  ui->powerPercentageEdit->setText(sString);
  commandVoltage = percentage/330.0;
  DAQWrite(commandVoltage);

  sendToAll(QString("<setPercent>%1</setPercent>").arg(sString));
  ui->applyButton->hide();
  return;

}


void
TControllerRPi::on_powerPercentageEdit_textChanged(const QString &arg1) {
  double pValue = arg1.toDouble();
  if((pValue < 0.0) || (pValue > 100.0)) {
    ui->powerPercentageEdit->setStyleSheet(sErrorStyle);
    ui->applyButton->hide();
    return;
  }
  ui->powerPercentageEdit->setStyleSheet(sNormalStyle);
  ui->applyButton->show();
}


void
TControllerRPi::onpowerPercentageReceiveded(double newValue) {
  if((newValue < 0.0) || (newValue > 100.0)) {
    return;
  }
  sString = QString("%1").arg(newValue, 0, 'f', 1);
  ui->powerPercentageEdit->setText(sString);
  ui->powerPercentageEdit->setStyleSheet(sNormalStyle);
  on_powerPercentageEdit_returnPressed();
}


void
TControllerRPi::on_applyButton_clicked() {
  on_powerPercentageEdit_returnPressed();
}
