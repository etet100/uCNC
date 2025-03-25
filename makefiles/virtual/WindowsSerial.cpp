// @GPILOT
#include <QtCore>
#include "WindowsSerial.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

WindowsSerial::WindowsSerial(const char *portName)
{
    Q_UNUSED(portName);

    this->socket = NULL;
    this->connected = false;
}

WindowsSerial::~WindowsSerial()
{
    //Check if we are connected before trying to disconnect
    if(this->connected && socket->isOpen())
    {
        this->socket->abort();
        delete this->socket;

        //We're no longer connected
        this->connected = false;
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

    this->connected = true;
}

