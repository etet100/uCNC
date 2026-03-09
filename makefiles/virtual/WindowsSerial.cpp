// @GPILOT
#include <QtCore>
#include "WindowsSerial.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

extern "C" {
    void gpilotLockProbeAtCurrentPosition(void);
    void gpilotResetProbePosition(void);
    void gpilotSetHome(bool abs, double x, double y, double z);
}

WindowsSerial::WindowsSerial(const char *portName)
{
    Q_UNUSED(portName);

    this->socket = NULL;
    this->connected = false;
}

WindowsSerial::~WindowsSerial()
{
    //Check if we are connected before trying to disconnect
    if (this->connected && socket->isOpen())
    {
        this->socket->abort();
        delete this->socket;

        //We're no longer connected
        this->connected = false;
    }

    if (controlSocket != nullptr) {
        this->controlSocket->abort();
        delete controlSocket;
    }
}

int WindowsSerial::ReadData(char *buffer, unsigned int nbChar)
{
    //Number of bytes we'll really ask to read
    unsigned int toRead;

    if (socket->bytesAvailable()) {
        //If there is we check if there is enough data to read the required number
        //of characters, if not we'll read only the available characters to prevent
        //locking of the application.
        toRead = MIN(nbChar, socket->bytesAvailable());

        qint64 bytesRead = socket->read(buffer, toRead);

        // //Handle "virtual settings" command (@@@,x,y,z,probe,estop)
        // if (bytesRead > 5 && buffer[0] == '@' && buffer[1] == '@' && buffer[2] == '@') {
        //     strpos(buffer, bytesRead, ",");

        //     qDebug() << "Received: " << buffer;
        // }

        return toRead;
    }

    static QString ctrlBuffer = "";
    if (controlSocket->bytesAvailable()) {
        ctrlBuffer += controlSocket->readAll();

        int pos;
        while ((pos = ctrlBuffer.indexOf("\n")) != -1) {
            QString line = ctrlBuffer.left(pos).trimmed();
            ctrlBuffer = ctrlBuffer.mid(pos + 1);

            qDebug() << "[WindowsSerial][Ctrl] Received:" << line;

            // Process control commands here
            QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                QString cmd = obj["cmd"].toString();

                if (cmd == "probe_at_current") {
                    gpilotLockProbeAtCurrentPosition();
                } else if (cmd == "reset_probe") {
                    gpilotResetProbePosition();
                } else if (cmd == "set_home") {
                    gpilotSetHome(
                        obj["abs"].toBool(),
                        obj["x"].toDouble(),
                        obj["y"].toDouble(),
                        obj["z"].toDouble()
                    );
                }
            }
        }
    }

    //If nothing has been read, or that an error was detected return 0
    return 0;
}

bool WindowsSerial::WriteData(const uint8_t *buffer, unsigned int nbChar)
{
    // qDebug() << "WriteData: " << (const char *)buffer;

    socket->write((const char *)buffer, nbChar);

    return true;
}

bool WindowsSerial::IsConnected()
{
    //Simply return the connection status
    return this->connected;
}

void WindowsSerial::connect(QString serverName)
{
    if (this->connected) {
        return;
    }

    socket = new QLocalSocket();
    socket->connectToServer(serverName);
    if (!socket->waitForConnected(100)) {
        // error
    }

    controlSocket = new QLocalSocket();
    controlSocket->connectToServer(serverName);
    if (!controlSocket->waitForConnected(100)) {
        // error
    }

    this->connected = true;
}

