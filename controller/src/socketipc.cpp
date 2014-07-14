/*
 *   Copyright (C) 2014 Pelagicore AB
 *   All rights reserved.
 */
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "socketipc.h"

static const int BUF_SIZE = 1024;

SocketIPC::SocketIPC(IPCMessage &message):
    m_message(message),
    m_socketPath(""),
    m_socket(0)
{
}

SocketIPC::~SocketIPC()
{
}

bool SocketIPC::initialize(const std::string &socketPath)
{
    m_socketPath = socketPath;

    if ((m_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        log_error() << "socket:" << strerror(errno);
        return false;
    }

    log_debug() << "Trying to connect to Pelagicontain...";

    struct sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, m_socketPath.c_str());
    socklen_t len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    if (connect(m_socket, (struct sockaddr *)&remote, len) == -1) {
        log_error() << "connect:" << strerror(errno);
        return true;
    }

    log_debug() << "Connected to Pelagicontain";

    if (send(m_socket, "hello", strlen("hello"), 0) == -1) {
        log_error() << "send:" << strerror(errno);
        return false;
    }

    return true;
}

bool SocketIPC::checkForMessages()
{
    char buf[BUF_SIZE];
    memset(buf, '\0', sizeof(char) * BUF_SIZE);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(m_socket, &readfds);
    select(m_socket + 1, &readfds, 0, 0, &tv);

    if (FD_ISSET(m_socket, &readfds)) {
        int received = recv(m_socket, buf, BUF_SIZE, 0);
        if (received <= 0) {
            log_error() << "recv:" << strerror(errno);
            // TODO: Do we need to propagate failure here?
            // TODO: Should we continue waiting for messages after this?
            return false; // Disconnects the signal handler
        }

        char c;
        int total = 0;
        char msg[BUF_SIZE];

        // Loop until the complete received buffer is processed
        bool done = false;
        while (!done) {
            memset(msg, '\0', sizeof(char) * BUF_SIZE);
            int i = 0;
            // Find (possibly many) null terminated messages in buffer
            do {
                c = buf[total + i];
                msg[i] = c;
                ++i;
            } while ((c != '\0') && (i != sizeof(buf)));

            total += i;

            log_debug() << "Received \"" << msg << "\"";
            std::string messageString(msg);

            bool messageProcessedOk = false;
            // If message is empty we don't need to handle it
            //TODO: Can it ever be empty? If only a null char was sent for example?
            if (messageString.size() > 0) {
                messageProcessedOk = m_message.handleMessage(messageString);
            }
            if (!messageProcessedOk) {
                // The message was not understood by IPCMessage
                // TODO: Is this a fatal failure? Propagate to shut down?
                log_info() << "Warning: IPC message to Controller was not sent" ;
            }

            if (total == received) {
                done = true;
            }
        }
    }

    return true; // Continue to let the signal handler call this method
}