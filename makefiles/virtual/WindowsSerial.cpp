// @GPILOT
#include <QtCore>
#include "WindowsSerial.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#include "../../uCNC/src/modules/astrocore_sim.h"

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

void WindowsSerial::processControlCommands()
{
    if (controlSocket == nullptr || !controlSocket->bytesAvailable()) {
        return;
    }

    ctrlBuffer += controlSocket->readAll();

    int pos;
    while ((pos = ctrlBuffer.indexOf("\n")) != -1) {
        QString line = ctrlBuffer.left(pos).trimmed();
        ctrlBuffer = ctrlBuffer.mid(pos + 1);

        qDebug() << "[uCNC] Received:" << line;

        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (doc.isNull() || !doc.isObject()) {
            continue;
        }

        QJsonObject obj = doc.object();
        QString cmd = obj["cmd"].toString();

        if (cmd == "probe_at_current") {
            astrocore_sim_probe_at_current();
        } else if (cmd == "reset_probe") {
            astrocore_sim_reset_probe();
        } else if (cmd == "set_home") {
            astrocore_sim_set_home(
                obj["abs"].toBool(),
                (float)obj["x"].toDouble(),
                (float)obj["y"].toDouble(),
                (float)obj["z"].toDouble()
            );
        } else if (cmd == "set_single_limit") {
            //{"axis":2,"cmd":"set_single_limit","pos":25}
            astrocore_sim_set_single_limit(
                obj["axis"].toInt(),
                (float)obj["pos"].toDouble()
            );
        } else if (cmd == "estop") {
            //"pressed" is optional so that the old fire-and-forget form still works
            astrocore_sim_estop(obj.contains("pressed") ? obj["pressed"].toBool() : true);
        } else if (cmd == "set_input") {
            //{"cmd":"set_input","mask":2048,"active":true}
            astrocore_sim_set_input(
                (uint16_t)obj["mask"].toInt(),
                obj["active"].toBool()
            );
        }
    }
}

int WindowsSerial::ReadData(char *buffer, unsigned int nbChar)
{
    //Always drain the control channel. Checking it only when the data socket is
    //idle used to starve it while g-code was streaming, which delayed estop.
    processControlCommands();

    if (socket->bytesAvailable()) {
        //If there is we check if there is enough data to read the required number
        //of characters, if not we'll read only the available characters to prevent
        //locking of the application.
        unsigned int toRead = MIN(nbChar, socket->bytesAvailable());

        socket->read(buffer, toRead);

        return toRead;
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

