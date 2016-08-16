#!/usr/bin/env python

""" copyright Pelagicore AB 2016 """

import os
import time
import subprocess
import Queue
import dbusapp
import unittest

from pydbus import SessionBus
from pelagipy import Receiver
from pelagipy import ContainerApp
from pelagipy import SoftwareContainerAgentHandler

class TestDBus(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.logFile = open("test.log", "w")
        time.sleep(0.5)

        """ Setting up dbus environement variables for session buss """
        p = subprocess.Popen('dbus-launch', shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for var in p.stdout:
            sp = var.split('=', 1)
            os.environ[sp[0]] = sp[1][:-1]

        cls.agentHandler = SoftwareContainerAgentHandler(cls.logFile)

    def grepForDBusProxy(self):
        return os.system('ps -aux | grep dbus-proxy | grep -v "grep" | grep prefix-dbus- > /dev/null')

    def test_query_in(self):
        """ Launch server in container and test if a client can communicate with it from the host system """
        for x in range(0, 10):
            ca = ContainerApp()
            try:
                ca.start()
                ca.dbusGateway()
                ca.launchCommand('{}/dbusapp.py server'.format(ca.getBindDir()))

                time.sleep(0.5)
                client = dbusapp.Client()
                client.run()
                self.assertTrue(client.check_all_good_resp())
            finally:
                ca.terminate()

    def test_query_out(self):
        """ Launch client in container and test if it communicates out """
        for x in range(0, 10):
            serv = dbusapp.Server()
            serv.start()
            ca = ContainerApp()
            try:
                ca.start()
                ca.dbusGateway()
                ca.launchCommand('{}/dbusapp.py client'.format(ca.getBindDir()))

                self.assertTrue(serv.wait_until_requests())
            finally:
                ca.terminate()
                serv.terminate()
                serv = None

    def test_spam_out(self):
        """ Launch client in container and stress test the communication out """
        ca = ContainerApp()
        try:
            serv = dbusapp.Server()
            serv.start()

            ca.start()
            ca.dbusGateway()

            clients = 100
            message_size = 8192 # Bytes

            t0 = time.time()
            for x in range(0, clients):
                ca.launchCommand('{}/dbusapp.py client --size {}'.format(ca.getBindDir(), message_size))
            t1 = time.time()
            self.assertTrue(serv.wait_until_requests(multiplier=clients))
            t2 = time.time()
            print("\n")
            print("Clients started:              {0:.4f} seconds\n".format(t1 - t0))
            print("Server received all messages: {0:.4f} seconds\n".format(t2 - t1))
            print("Total time:                   {0:.4f} seconds\n".format(t2 - t0))

        finally:
            ca.terminate()
            serv.terminate()
            serv = None

    def setUp(self):
        self.assertNotEqual(self.grepForDBusProxy(), 0, msg="dbus-proxy not shutdown")

    def tearDown(self):
        self.assertNotEqual(self.grepForDBusProxy(), 0, msg="dbus-proxy not shutdown")

    @classmethod
    def tearDownClass(cls):
        cls.agentHandler.terminate()
        cls.logFile.close()

if __name__ == "__main__":
    unittest.main()