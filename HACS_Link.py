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

import RPi.GPIO as GPIO

"""
 All switch logic is fail-safe, meaning if power is disconnected through the
 switch (logical 0), the safest action will occur.

 # RS IS DEPRECATED DUE TO SWITCH HARDWARE PROBLEM
"""

GPIO.setmode(GPIO.BCM) # Broadcom pin definitions

class HACS_Link(object):
    def __init__(self):
        """ HACS Link switches on/off various GPIO pins so that the HACS-ADCS
            companion Microcontroller can read them and send their state
        """

        # HACS BCM Pin Role Defintions
        self.pin_roles = {
                            20: 'E',     # Emergency STOP
                            5 : 'a',      # Arm
                            16: 'F1',     # Fire Suppression 1
                            #26: 'RS', # ADCS Software_Reset
                          }

        # Defines what value constitutes as ACTIVE (if pin N.O. or N.C.)
        self.role_default = {
                            'E':0,
                            'a':0,
                            'F1':1,
                            #'RS':1,
                            }

        # Defines the role assigned to each annunciator light pin
        self.annunciator_roles = {12:'E'}

        self.pin_states={}
        self.last_pin_states={}

        GPIO.setwarnings(False)

        # Input Setup
        for pin in self.pin_roles:
            self.pin_states[pin]      = 0
            self.last_pin_states[pin] = 1 # Force update on first run
            GPIO.setup(pin, GPIO.IN)      # Enable input pullups

        # Output setup
        for pin in self.annunciator_roles:
            GPIO.setup(pin, GPIO.OUT)     # Enable pin as output

    def update(self,state):
        """ Perform an update of all output annunciators and input cmds """

        cmd = {}

        # Take inputs
        for pin in self.pin_roles:
            self.pin_states[pin] = GPIO.input(pin)

        # Write output states
        for pin,role in self.annunciator_roles.items():
            GPIO.output(pin,(self.role_default[role] == int(state[role]) ))

        # Update command dictionary with inputs
        for pin,role in self.pin_roles.items():
            if self.last_pin_states[pin] != self.pin_states[pin]:
                cmd[role] = int(self.pin_states[pin]==self.role_default[role])

        self.last_pin_states = dict(self.pin_states)

        return cmd
