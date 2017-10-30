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

from multiprocessing import Process, Pipe
from Definitions import State_Limits
from Microlink import Microlink
import Adafruit_DHT as DHT
import RPi.GPIO as GPIO
from colorama import *
from time import time
import socket
import sys,os

###                                                              ###
###  ELECTRICAL FUNCTIONS - Switching On/Off the AC power relay  ###
###                                                              ###
class Electrical(object):
    def __init__(self):
        ''' Initialize GPIO to control the electrical control pin '''

        self.pin = 24
        self.state = 0

        GPIO.setwarnings(False)
        GPIO.setmode(GPIO.BCM) # Broadcom pin definitions
        GPIO.setup(self.pin, GPIO.OUT)

    def Electrical_Activate(self):
        ''' Power up both electrical buses '''

        if not self.state:
            self.state = 1
            GPIO.output(self.pin, self.state)

    def Electrical_Deactivate(self):
        ''' Power down both electrical buses '''

        if self.state:
            self.state = 0
            GPIO.output(self.pin, self.state)

###                                                              ###
###  Software based reset and shutdown functions                 ###
###                                                              ###

def Micro_Reset(microlink):
    ''' Reset the microcontrollers via Microlink re-init '''
    now_connected = list(microlink.order)
    microlink.Microdownlink_Disconnect()

    print('Reconnecting to ',now_connected)
    microlink = Microlink(now_connected,strict=True)

def Software_Reset(actions):
    ''' Reset this program '''
    print('>> ADCS SOFTWARE RESET <<')

    actions.micro.Microdownlink_Disconnect()
    actions.hdcs.tcp_close()
    actions.Thermal.Stop()

    python = sys.executable
    os.execl(python, python, * sys.argv)

def Software_Exit(actions):
    """ Cleanly shut off this program """
    print('>> ADCS SOFTWARE EXIT <<')

    actions.micro.Microdownlink_Disconnect()
    actions.hdcs.tcp_close()
    actions.Thermal.Stop()

    sys.exit()

def ADCS_SHUTDOWN():
    ''' Force operating system to shut down (USE ONLY in critical states!) '''
    print(Fore.BLUE+Back.WHITE+'                                               ')
    print(Fore.BLUE+Back.WHITE+' >-- INITIATING ADCS AUTO SHUTDOWN --<')
    print(Fore.BLUE+Back.WHITE+'                                               ')
    os.system("sudo shutdown -h now")

###                                                                     ###
###  Internal electronics temperature sensing via DHT & multiprocessing ###
###                                                                     ###

class Thermal_Sensing(object):
    def __init__(self):
        ''' This class detects the internal electronics temp, and takes action'''
        self.lastTemp,self.lastHalt = -1,0,
        self.lastTempCheckTime,self.time_over_limits,self.rxTime = time(),time(),time()
        self.A, self.B = Pipe()
        self.Temp_Prc = Process(target=self.Temperature_Check,args=(self.B,))
        self.Temp_Prc.start()

    def Stop(self):
        self.Temp_Prc.terminate()

    def Temperature_Check(self,link):
        ''' This loop runs within a separate process to preserve asynchronosity'''
        while True:
            if link.poll()>0:
                link.recv()
                humidity, temp = DHT.read(11, 18)
                link.send(temp)
            else:
                link.send(None)

    def Eval(self,Command,State):
        ''' Evaluate received temp, and shutdown if necessary '''
        temperature = None
        halt = 0

        if time()-self.lastTempCheckTime > 5:
            self.lastTempCheckTime=time() # only ping every 5 sec, but
            self.A.send('tigger')

        if self.A.poll()>0:
            temperature = self.A.recv()
            self.rxTime = time()

        if temperature is not None:

            if temperature >= State_Limits['it'][1]:
                halt = 1

            State['it'] = temperature # use a link here, not a copy

        else:

            halt        = self.lastHalt

        if time()-self.rxTime > 10:
            print(Fore.WHITE+Back.RED+'!! ADCS THERMAL FAILED TO RESPOND !!')

        if self.lastHalt != halt:
            self.time_over_limits = time()

        if halt==1:
            print(Fore.WHITE+Back.RED+
            '!! HOLD HOLD HOLD - ADCS CRITICAL TEMP AT ',temperature,' DEG C FOR '
                                +str(time() - self.time_over_limits)+' SECONDS')
            Command['E'] = 1
            Command['a'] = 0
            Command['Cm']= 0

        elif halt==0:
            self.time_over_limits = time() # if we go back under halt limits, this deactivates shutdown

        if time() - self.time_over_limits >= 10:
            print(Fore.RED+Back.WHITE+
            '!! ADCS CRITICAL TEMPERATURE EXCEEDED FOR '+str(time() - self.time_over_limits)
                                                  +'SECONDS !!! AUTO SHUTDOWN')
            ADCS_SHUTDOWN()

        self.lastHalt=halt
