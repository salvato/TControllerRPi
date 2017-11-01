#ifndef TCONTROLLERRPI_H
#define TCONTROLLERRPI_H

#include <QMainWindow>
#include <QUrl>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QWebSocket>
//#include <QTimer>
#include <QSettings>

QT_FORWARD_DECLARE_CLASS(QWebSocket)
QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QFile)
QT_FORWARD_DECLARE_CLASS(NetServer)
QT_FORWARD_DECLARE_CLASS(ServerDiscoverer)
QT_FORWARD_DECLARE_CLASS(QUdpSocket)
QT_FORWARD_DECLARE_CLASS(ClientListDialog)
QT_FORWARD_DECLARE_CLASS(MCP4725)


namespace Ui {
class TControllerRPi;
}

class TControllerRPi : public QMainWindow
{
    Q_OBJECT

public:
  explicit TControllerRPi(QWidget *parent = 0);
  ~TControllerRPi();

private slots:
  void onExitProgram();
  void on_powerPercentageEdit_returnPressed();
  void on_powerPercentageEdit_textChanged(const QString &arg1);
  void onpowerPercentageReceiveded(double newValue);
  void on_applyButton_clicked();
  void onProcessTextMessage(QString sMessage);
  void onProcessBinaryMessage(QByteArray message);
  void onProcessConnectionRequest();
  void onClientDisconnected();
  void onPanelServerError(QWebSocketProtocol::CloseCode closeCode);
  void onNewPanelServerConnection();
//  void onTimeToUpdate();

protected:
  void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
  bool isConnectedToNetwork();
  void prepareDiscovery();
  int  prepareServer();
  bool prepareLogFile();
  void startDAQ();
  int  sendAcceptConnection(QUdpSocket *pDiscoverySocket, QHostAddress hostAddress, quint16 port);
  void RemoveClient(QHostAddress hAddress);
  void UpdateUI();
  virtual QString FormatStatusMsg();
  int  sendToAll(QString sMessage);
  int  sendToOne(QWebSocket* pSocket, QString sMessage);
  void closeServer();
  void CloseDAQ();
  void DAQWrite(double dValue);

private:
  Ui::TControllerRPi *ui;

  struct connection {
    QWebSocket*     pClientSocket;
    QHostAddress    clientAddress;
  };

  QString     sString;
  QString     sNormalStyle;
  QString     sErrorStyle;
  QString     sAverage;

  QSettings   settings;

  MCP4725*              pDAC;
  double                percentage;
  double                commandVoltage;

  QFile                *logFile;
  QWebSocket           *pServerSocket;
  QWebSocketServer     *pPanelServer;
  ServerDiscoverer     *pServerDiscoverer;
  ClientListDialog     *pClientListDialog;

  quint16               discoveryPort;
  QHostAddress          discoveryAddress;
  quint16               serverPort;
  QVector<QUdpSocket*>  discoverySocketArray;

  QStringList           sIpAddresses;
  QString               logFileName;
  QList<connection>     connectionList;

//  QTimer                updateTimer;

};

#endif // TCONTROLLERRPI_H
