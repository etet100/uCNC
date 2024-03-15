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
    if(this->connected)
    {
        socket->close();
        socket->waitForDisconnected();
        socket->deleteLater();

        //We're no longer connected
        this->connected = false;
    }
}

int WindowsSerial::ReadData(char *buffer, unsigned int nbChar)
{
    //Number of bytes we'll really ask to read
    unsigned int toRead;

    // socket->waitForReadyRead(0);
    if (socket->bytesAvailable()) {
        //If there is we check if there is enough data to read the required number
        //of characters, if not we'll read only the available characters to prevent
        //locking of the application.
        toRead = MIN(nbChar, socket->bytesAvailable());

        socket->read(buffer, toRead);

        return toRead;
    }

    //If nothing has been read, or that an error was detected return 0
    return 0;
}

bool WindowsSerial::WriteData(const uint8_t *buffer, unsigned int nbChar)
{
    // if (socket->bytesToWrite()) {
    //     socket->waitForBytesWritten(5);
    // }
    socket->write((const char *)buffer, nbChar);
    // socket->flush();

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
    // socket->write(QString("test").toStdString().c_str());
    // socket->waitForBytesWritten();

    this->connected = true;
}

