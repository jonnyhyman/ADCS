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
import Actuate as act
from   time    import sleep
from itertools import groupby

class Actions(object):
    def __init__(self,micro,hdcs):
        """ This class plays two critical roles:
                1. Take manual control actions via the Cmd structure
                2. Take auto control actions via instrumentation limits
        """
        # Instantiate classes
        self.Thermal  = act.Thermal_Sensing()
        self.Electrical = act.Electrical()

        # Initialize variables
           # Needs link to micro for upload control & nexus
        self.micro    = micro
        self.hdcs     = hdcs
        self.last_cmd = {}

    def Cmd(self,cmd,state):
        """ Check if commands are different than last time, & if so, act """

        if self.last_cmd != cmd:

            # Microcode upload mode
            if cmd['UC']:

                programs = cmd['UC'].split('&')
                programs.pop(-1) # last entry will always be ''

                self.micro.Microuplink(programs)

                state['UC']=cmd['UC'] # send ack to ADCS
                cmd['UC']=0 # reset until further notice

                print('>>UC COMMAND RESET<<')

            # Device reset mode
            if cmd['RS']:

                if cmd['RS'] == 'ADCS' or cmd['RS'] == 1:
                    act.Software_Reset(self)

                elif cmd['RS'] == 'MICRO':
                    act.Micro_Reset(self.micro)

                elif cmd['RS'] == 'ADCS_OFF':
                    act.Software_Exit(self)

                state['RS'] = cmd['RS'] # send ack to ADCS, if still alive
                cmd['RS']   = 0         # reset flag to until further notice

            # Test matrix sequence & Nexus upload
            if cmd['TM'] and not state['TM']:

                print('>>TM CMD INIT ACK<<')

                state['TM'] = 1    # AcknowLedge the command
                self.TestMatrix = [] # Reset the test matrix

            elif not cmd['TM'] and state['TM']:

                print('>>TM DEACTIVATED<<>>MOVING TO NEXUS')

                # Omit duplicate rows
                self.TestMatrix = [row for row,val in # using itertools
                                   groupby(sorted(self.TestMatrix))]

                print("OPERATOR VERIFY MATRIX>>",self.TestMatrix)
                sleep(10) # Pause so that operator can verify the matrix

                state['TM'] = 0

                self.micro.Nexusuplink(self.TestMatrix,'Micro1')
                self.micro.Nexusuplink(self.TestMatrix,'Micro2')

                # Command to set countdown clock & hold
                cmd['c1'] =self.TestMatrix[0][0]
                cmd['c1_']=self.TestMatrix[0][1]
                cmd['c2'] =self.TestMatrix[0][0]
                cmd['c2_']=self.TestMatrix[0][1]
                cmd['Cm']=2

            # Bus power-up actuation
            if cmd['B1'] or cmd['B2']:
                self.Electrical.Electrical_Activate()
                state['B1'] = 1
                state['B2'] = 1

            elif not cmd['B1'] or not cmd['B2']:
                self.Electrical.Electrical_Deactivate()
                state['B1'] = 0
                state['B2'] = 0


            self.last_cmd = dict(cmd)

        if cmd['TM'] and state['TM']:
            if cmd['TM']!=1:
                if ( len(self.TestMatrix) == 0 or cmd['TM']!=self.TestMatrix[-1]):
                    print('>>TM APPEND<<',  cmd['TM'],'index',state['TM'])
                    self.TestMatrix.append( cmd['TM'] )
                    state['TM'] += 1 # increment index

    def Auto(self,cmd,state,inhibit):
        """ Check if exceeded parameters & if so, act.
            NOTE: This function modifies cmd & state in-place, no return """

        self.Thermal.Eval(cmd,state)


        if inhibit != 'auto':
            if (state['C1']>=-20 or state['C2']>=-20): # ADCS TERMINAL COUNT:
                for key,limits in Definitions.State_Limits.items() :
                    if state[key] >= limits[1]:
                        cmd['E'] = 1
                        state['ER'] = ''.join([key,'HI'])
                        break

                    elif state[key] <= limits[0]:
                        cmd['E'] = 1
                        state['ER'] = ''.join([key,'LO'])
                        break
