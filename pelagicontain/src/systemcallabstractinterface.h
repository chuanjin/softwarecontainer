/*
 *   Copyright (C) 2014 Pelagicore AB
 *   All rights reserved.
 */
#ifndef SYSTEMCALLABSTRACTINTERFACE_H
#define SYSTEMCALLABSTRACTINTERFACE_H

#include <string>

/*! SystemcallAbstractInterface provides an abstract interface to the system.
 *
 *  This class is used by Pelagicontain to communicate with the host system
 *  and is intended to be an abstraction of the acutal implementation.
 */
class SystemcallAbstractInterface {

public:

    virtual ~SystemcallAbstractInterface() {
    };

    /*! Issues a call to system() with the command passed as argument
     *
     * \param cmd The command sent to system().
     * \return True if the command was successfully executed.
     */
    virtual bool makeCall(const std::string &cmd) = 0;

    /*! Issues a call to system() with the command passed as argument
     *
     * \param cmd The command sent to system().
     * \param exitCode Stores the exit code returned by the call to system()
     * \return True if the command was successfully executed.
     */
    virtual bool makeCall(const std::string &cmd, int &exitCode) = 0;

};

#endif /* SYSTEMCALLABSTRACTINTERFACE_H */