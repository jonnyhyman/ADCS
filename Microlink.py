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

import Definitions
import serial.tools.list_ports
import serial

from time import time,sleep
from colorama import *
from subprocess import call
from threading import Thread
from multiprocessing import Process, Pipe

class Microlink(object):
    def __init__(self,micros,strict=False):
        """ Microlink class runs a robust serial link to Microcontrollers """

        self.std_baud = 250000

        # Micro ports are the serial ports associated with each micro
        self.micro_ports={}
        for mu in micros:
            self.micro_ports[mu]    = ''     # as yet unknown ports

        if not strict:
            # A non-strict initialization won't exit if only SOME micros found
            self.Bonjour(verbose=False)
        else:
            # If strict, then keep trying until all micros found
            found_len, targ_length = 0,len(self.micro_ports)
            while (found_len!=targ_length):

                self.micro_ports={}
                for mu in micros:    # dynamic allocation of micro attributes
                    self.micro_ports[mu]    = ''     # as yet unknown ports

                self.Bonjour(verbose=True)
                found_len = len(self.micro_ports)

        self.lastCmd      = Definitions.Command
        self.last_newCmds = Definitions.Command
        self.downlinking  = False
        self.connected    = False
        self.serialconn   = {}
        self.order        = []

    def Connect_And_Reset(self,port,wait=1,timeout=5):
        """ Force DTR line OFF then ON to reset, and reconnect """

        conn = serial.Serial(port,self.std_baud)
        conn.setDTR(False)
        sleep(wait)        # give micro time to reset
        conn.setDTR(True)
        conn.flushInput()
        sleep(wait/2)
        conn.timeout=timeout
        return conn

    def Connect_And_Reset_realtime(self,port,link,wait=1,timeout=3):
        """ Force DTR line OFF then ON to reset, and reconnect + maintain
            the microlink process for that port
        """
        conn = serial.Serial(port,self.std_baud)
        conn.setDTR(False)
        start=time()
        while (time()-start < wait): # Don't hold the whole dang program!
            if link.poll():
                link.recv()
                link.send('-1')
        conn.setDTR(True)
        conn.flushInput()
        start = time()
        while (time()-start < wait/2): # Don't hold the whole dang program!
            if link.poll():
                link.recv()
                link.send('-1')
        conn.timeout=timeout
        return conn

    def Bonjour_Core(self,p,verbose):
        """ The core function for performing "Bonjour", a Microcontroller
            hostname resolution / identification program
        """
        notChecked=True
        timeout   =False

        print(' ... checking',p[0])
        micro=''
        conn = self.Connect_And_Reset(p[0])
        start=time()

        # Now that we're connected, listen for micro identifier
        while (notChecked and timeout==False):
            line = conn.readline()
            line = line[:-2] # get rid of \r\n
            if verbose:
                print('raw:',line)
            try:
                line = line.decode('utf-8',errors='ignore') # errors allows for
                if ('-->' in line):                         # detection even if
                    micro = line[line.index('-->')+3:]      # invalid utf chars
                    if verbose:                             #(especially '\xfe')
                        print('utf:',line)
                        print('det:',micro)
                    if micro in self.micro_ports.keys():
                        self.micro_ports[micro]=p[0]
                    conn.close()
                    notChecked=False
                else:
                    conn.close()
                    conn = self.Connect_And_Reset(p[0])
            except UnicodeDecodeError:
                # occurs if received data bytes are mangled
                pass

            if time()-start>conn.timeout:
                print(Fore.MAGENTA+p[0]+' is not a Bonjour device')
                timeout=True

    def Bonjour(self,verbose=False):
        """ Perform a multithreaded Bonjour across all connected devices """

        self.ports = list(serial.tools.list_ports.comports())

        ts = [n for n in range(len(self.ports))]
        for p in range(len(ts)):
            ts[p] = Thread(target=self.Bonjour_Core,args=(self.ports[p],verbose,))
            ts[p].start()

        for t in ts:
            t.join()

        """ If couldn't find a micro, don't list it! """
        deletekeys=[]
        for key,value in self.micro_ports.items():
            if value=='':
                deletekeys.append(key)

        for k in deletekeys:
            del self.micro_ports[k]

        print(' ')
        for key,value in self.micro_ports.items():
            print(Fore.GREEN+'Bonjour found: '+Fore.WHITE+str(key)+' '+str(value))
        print(Style.RESET_ALL+' ')

    def Microuplink(self,programs):
        """ Controller function for program upload.
              - Do threaded uplink of new Microcode to Microcode namesake Micros
        """

        print('Microuplink for',programs)

        if self.downlinking:
            reconnect_after_upload = True
            self.Microdownlink_Disconnect()

        upThreads=[]
        self.micro_programs={}
        for program in programs:
            self.micro_programs[program] = program
            upThreads.append(0)

        startT=time()
        for n,key in enumerate(self.micro_programs):
            upThreads[n] = Thread(target=self.Upload,args=(self.micro_programs[key]
                                                          ,self.micro_ports[key],))
            upThreads[n].start()

        for t in upThreads:
            t.join()

        print(Fore.BLUE+Back.WHITE+'Threaded Uploads Complete, took '+str(time()-startT)+' sec',end='')
        print(Style.RESET_ALL)

    def Upload(self,filename,port):
        """ Perform a re-flashing to a micro of new codes. This function takes
            a tremendously long time to complete, because it initializes Java
            in the background (for Arduino IDE) ... Screw you Java!
        """

        if port not in [p[0] for p in self.ports]:
            print(Fore.RED+'Port Fail')
        else:
            filepath = './Microcode/'+filename+'/'+filename+'.ino'
            execpath = '/usr/share/arduino/arduino' # path to executable

            command  = 'sudo '                   # must be done to allow build.path
            command += execpath                  # 'arduino' execution full path
            command +=' --upload --port '
            command += port                      # port setting
            command +=' --pref build.path=/ard-builds-'+filename+'/ --verbose '
            command += filepath                  # name of program to upload
            print(' ')
            print(Fore.GREEN+command)
            print(' ')
            exitcode = call(command, shell=True) # subprocess.call

    def Nexusuplink(self,TestMatrix,Micro):
        """ Nexus is the serial transport-layer protocol for robust uplink
            of data to Microcontrollers, in our case, the Test Matrix """

        n = 0
        mi = self.order.index(Micro) # figure out who is the target (what index)
        mid= Micro[Micro.index('Micro')+5:] #  who is the target (micro id #)

        step  = 0

        Nexus = True
        while Nexus:

            if step==0:
                print(Fore.WHITE+'Nexus:'+mid+',')
                self.microport.send('Nexus:'+mid+',')
                self.microport.recv()[0][mi] # get perfunctory byte
                step=1 #--> potential weakness (no margin for missing Nexus:mid,)

            if step==1:

                row = TestMatrix[n]
                row_str = ''            # build row as a string
                for item in row:
                    row_str+=str(int(item))
                    row_str+=','

                print(Fore.WHITE+row_str)
                self.microport.send(row_str)
                got=self.microport.recv()[0][mi]
                print(Fore.CYAN+got)

                if row_str in got: # rather than == because sometimes bytes left from reporting
                    n+=1
                    if n>len(TestMatrix)-1:
                        print(Fore.WHITE+'BYE')
                        self.microport.send('BYE')
                        got=self.microport.recv()[0][mi]
                        print(Fore.CYAN+got)
                        if got not in 'BYE': # sometimes only got =='BY'
                            print(Fore.RED+Back.WHITE+"Micro didn't respond with echo BYE")
                            sleep(5)
                            #raise ValueError("Micro didn't respond with echo BYE")
                        else:
                            Nexus=False
                    else:
                        print(Fore.WHITE+'RGR')
                        self.microport.send('RGR')
                        got=self.microport.recv()[0][mi]
                        print(Fore.CYAN+got)
                        if got not in 'RGR': # sometimes only got =='RG'
                            print(Fore.RED+Back.WHITE+"Micro didn't respond with echo RGR")
                            sleep(5)
                            #raise ValueError("Micro didn't respond with echo RGR")
                else:
                    print(Fore.WHITE+'NEG')
                    self.microport.send('NEG')
                    got=self.microport.recv()[0][mi]
                    print(Fore.CYAN+got)
                    if got not in 'NEG': # sometimes only got == 'NE'
                        print(Fore.RED+Back.WHITE+"Micro didn't respond with echo NEG")
                        sleep(5)
                        #raise ValueError()

    def Microcom(self,link,port):
        """ Microcom is called as a multitude of processes, and they are
            responsible for handling all inbound and outbound serial traffic
            from their com ports
        """
        conn=0
        connected=False
        while True:
            if not connected:
                try:
                    conn = self.Connect_And_Reset_realtime(port,link,timeout=0.02)
                    conn.write_timeout = 0.01
                    if conn.isOpen():
                        connected=True

                    else:
                        if len(link.recv())>0: # doing this synchs serial after
                            print('except 1') # a disconnect->reconnect situation
                            link.send('-1')
                        conn.close()
                        connected=False

                except serial.serialutil.SerialException:
                    if len(link.recv())>0:
                        print('except 2')
                        link.send('-1') # keep the upper link alive and changing

                    pass # this occurs if the micro is disconnected after bonjour
            else:
                line = ''
                try:

                    if conn.inWaiting(): # clear input buffer but send if pertinent
                        print(Fore.GREEN+port+'-->',end='')
                        while conn.inWaiting():
                            print(Fore.GREEN+conn.read().decode('utf-8',errors='ignore'),end='')
                            conn.read()
                        print('')

                    # OPTION 1: ~66 HZ, solid clean data (usually)
                    ctrl = link.recv()
                    conn.write(bytes(ctrl+'\n','utf-8'))
                    line = conn.readline()

                    if (line!=''):
                        line = line.decode('utf-8',errors='ignore')
                        line = line.replace('\r\n','') # garbage collection :)
                        line = line.replace(' ','')
                        if (line!=''):
                            #print(line)
                            link.send(line)

                        else:
                            #print('empty 1')
                            link.send('-1')
                    else:
                        #print('empty 2')
                        link.send('-1')

                except (serial.serialutil.SerialTimeoutException) as e:
                    print(e)
                    conn.reset_input_buffer()
                    conn.reset_output_buffer()
                    link.send('-1')

                except (serial.serialutil.SerialException) as e:
                    #print('Board at ',port,'failed to read')
                    print(e)
                    link.send('-1')
                    conn.close()
                    connected=False

                except (UnicodeDecodeError) as e:
                    # this occurs if the .decode() failed
                    #print('Board at ',port,'sent garbled bits')
                    print(e)
                    link.send('-1')

    def Microport(self,link):
        """ Microport is called as a process, and it is responsible for handling
            all inbound and outbound serial traffic from com handlers (Microcoms)
        """

        ports_established=False
        while True:
            if not ports_established:

                (self.A,self.B,
                 self.process,self.datas,
                 self.order_)=[],[],[],[],[]

                for n, micro_port in enumerate(self.micro_ports.items()):

                    micro, port = micro_port

                    # prepare list entries
                    self.process.append(0)
                    self.A.append(0)
                    self.B.append(0)
                    self.datas.append(0)
                    self.order_.append(micro)

                    # open pipes and processes for microcom
                    self.A[n], self.B[n] = Pipe()
                    self.process[n] = Process(target=self.Microcom,
                                              args=(self.B[n],port,))
                    self.process[n].daemon=True
                    self.process[n].start()

                link.send(self.order_)
                ports_established=True

            else:
                # Data to send to Micros comes from the pipe
                send_to_micros = link.recv()

                if len(send_to_micros)>0:

                    for com in self.A:
                        com.send(send_to_micros)

                    recv_times = []
                    for n, conn in enumerate(self.A):
                        start = time()
                        self.datas[n] = conn.recv()
                        recv_times += [time()-start]

                    link.send([self.datas,recv_times])

                else:
                    print(-1)
                    return '-1'

    def Microevent(self,Command,State,show=False):
        """ Microdownlink assigns a process to handle I/O from all micros """

        if not self.downlinking:
            """ Initialize if not initialized """
            self.microport, self.microlink  = Pipe()
            self.downlink = Process(target=self.Microport, args=(self.microlink,))
            self.downlink.start()
            self.downlinking=True
            self.order = self.microport.recv()
            print('Order: ',self.order)

        else:
            """ Check if any new commands versus last time """
            # Add commands which were not properly actuated (microlink error)
            newCmds = { key:val for key,val in self.last_newCmds.items()
                                if State[key] != val }

            newCmds.update({ key:value for key,value in Command.items()
                             if value!=self.lastCmd[key] and (type(value)==float
                                                           or type(value)==int)})

            # Add commands which need to be included with others ("key teams")
            #for leader, team in Definitions.Key_Teams.items():
            #    if leader in newCmds:
            #        for key in team:
            #            newCmds[key] = Command[key]
            #            print('added',key,'to newCmds')
            #    else:
            #        print(leader,'not in newCmds')

            if newCmds!={}:
                """ Only send commands if they are not the same as last time """
                self.lastCmd = dict(Command)
                self.last_newCmds = dict(newCmds)
            else:
                """ If no new Commands to send, only trigger """
                newCmds = {'T':1}


            # This join method performs ~1700 key,value pairs / millisecond
            ToMicros = ''.join(['%s:%s,' % (key, int(value))
                          for (key, value) in newCmds.items()])

            print('ToMicros-->',ToMicros)

            self.microport.send(ToMicros)
            data = self.microport.recv()

            if show:
                print(Style.RESET_ALL)
                colors = [Back.CYAN,Back.WHITE,Back.MAGENTA,Back.YELLOW]
                for n, item in enumerate(data[0]):
                    print(colors[n]+Fore.BLACK+' '+str(item)+' ',end='')
                print(Style.RESET_ALL+' ')

            return data

    def Microdownlink_Disconnect(self):
        self.downlink.terminate()
        self.downlinking=False
