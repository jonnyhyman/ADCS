"""
    This file is part of ADCS.

    ADCS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ADCS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ADCS.  If not, see <http://www.gnu.org/licenses/>.
"""

import socket
import ast

"""-----------------------------------------------------------------------------
             Primary HDCS Link occurs via TCP in the main process
-----------------------------------------------------------------------------"""

class HDCS_Link(object):
    def __init__(self):
        """ Create TCP socket, bind, and try to initiate connection """

        self.tcp_host = ''
        #self.tcp_port = 50010
        self.tcp_port = 50030

        self.tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.tcp.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.tcp.settimeout(0.001)

        print('tcp binding...')

        while True:

            try:
                self.tcp_port = 50010
                self.tcp.bind((self.tcp_host, self.tcp_port))
                break
            except socket.error:
                try:
                    self.tcp_port = 50020
                    self.tcp.bind((self.tcp_host, self.tcp_port))
                    break
                except socket.error:
                    try:
                        self.tcp_port = 50030
                        self.tcp.bind((self.tcp_host, self.tcp_port))
                        break
                    except socket.error:
                        pass
                pass

        self.connected = False
        self.tcp.listen(1)
        self.tcp_connect()

        print('tcp Host:',self.tcp_host,'tcp Port:',self.tcp_port)

    def tcp_connect(self):
        """ Attempt to connect to HDCS via TCP """
        #print('tcp LOS')
        try:
            try:
                self.tcp_connection.close()
                self.tcp_connection,self.addr = self.tcp.accept()
                self.connected = True
                print('tcp AOS')

            except (AttributeError):
                self.tcp_connection,self.addr = self.tcp.accept()
                self.connected = True
                print('tcp AOS')

        except (socket.timeout):
            if self.connected:
                # Only do one
                print('tcp LOS, timeout')

            self.connected = False

    def tcp_close(self):
        self.tcp.shutdown(socket.SHUT_RDWR)
        self.tcp.close()

    def receive(self):
        """ Receive cmd string from connection """
        if self.connected:
            try:
                self.tcp_connection.settimeout(0.001)
                return self.tcp_connection.recv(4096)
            except (socket.timeout,ConnectionResetError) as e:
                #self.connected = False
                return None

    def cmd_parser(self,data):
        """ Parse the latest received cmd data string into a dict """

        data = data.decode()

        # The TCP data has old data mixed in, so be sure to only get the latest
        start,end = data.rfind('{'), data.rfind('}')+1 # INCLUDE BRACES

        if start>=end:
            #If TCP got only string with } BEFORE {, then EOF will occur
            # Get the actual start, BEFORE the last }
            start = data.rfind('{',0,end)

        new_data  = data[start:end]

        try:
            return ast.literal_eval(new_data)
        except Exception as e:
            print(e)
            pass

    def send(self,state):
        """ Send state dictionary to connection """
        if self.connected:
            try:
                self.tcp_connection.sendall(bytes(str(state),'utf-8'))
            except (ConnectionResetError, BrokenPipeError,socket.timeout) as e:
                self.connected = False
                print(e)
        else:
            self.tcp_connect()

    def transceive(self,state):
        """ Receive + Send from the TCP connection """
        cmd = self.receive()
        self.send(state)
        return cmd

    def transceive_and_parse(self,state):
        """ Do both receiving and parsing, and return the new cmds """
        if self.connected:

            data = self.transceive(state)
            if data is not None:
                return self.cmd_parser(data)
            # else return None
        else:
            self.tcp_connect()
