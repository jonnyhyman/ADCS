"""---------------------------------------------------------------------------

Copyright (C) 2017, Jonathan "Jonny" Hyman

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

-----------------------------------------------------------------------------

Rocket Project
Autonomous Digital Control Station
https://github.com/jonnyhyman/ADCS

    - HDCS Communications
    - Microcontroller robust duplex communications ("Microlink")
    - Micro reliable test matrix uploads ("TM")
    - Micros remote upload triggering  ("UC")
    - Automatic exceeded range aborts ("ER")
    - Countdown control (Halt,Start,Set)
    - HACS robust duplex communications

-----------------------------------------------------------------------------
"""

VERSION = 6.8

from time import time
import sys

try:
    inhibit = str(sys.argv[1]).lower()
except IndexError:
    inhibit = None

if inhibit is not None:
    # >> options are:
    #    -- n2o: to inhibit wireless sensing
    #    -- auto: to inhibit auto command actions
    print('>> ADCS INHIBIT ',inhibit,'<<')

import Definitions
from State_Generator import State_Generator
from Telemetry_link import Wireless_Sensing
from Microlink import Microlink
from HDCS_Link import HDCS_Link
from HACS_Link import HACS_Link
from Actions import Actions
from Logging import Log

if __name__ == '__main__':

    """ Instantiate classes """
    micro = Microlink(['Micro1','Micro2','Micro3','Micro4'],strict=True)

    if inhibit != 'n2o':
        n2o   = Wireless_Sensing(
                                       port = Definitions.N2O['port'],
                                   mac_addr = Definitions.N2O['mac'],
        #                                 ip = Definitions.N2O['ip'],
                                )

    state = State_Generator()
    hdcs  = HDCS_Link()
    hacs  = HACS_Link()
    act   = Actions(micro,hdcs)

    """ Initialize variables """
    Command = dict(Definitions.Command)
    State   = dict(Definitions.State)
    log     = Log([State,Command])

    while True:

        # Acquire human input and update commands
        hdcsCmd = hdcs.transceive_and_parse(State)
        hacsCmd = hacs.update(State)

        if hdcsCmd:
            Command.update(hdcsCmd)
            print('hdcs cmd update! >>',hdcsCmd)

        if hacsCmd != {}:
            Command.update(hacsCmd)
            print('hacs cmd update! >>',hacsCmd)

        # ADCS Command Actions
        act.Cmd(Command, State)

        # Micro Command Actions -> Update State
        state.Generator(State, micro.Microevent(Command,State,show=1), micro.order)

        # Telemetry State Update
        if inhibit != 'n2o':
            state.Telemetry(State, [n2o], verbose=0)

        # ADCS Auto Command Actions
        act.Auto(Command,State,inhibit)

        log.Commit()
