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

class State_Generator(object):
    def __init__(self):
        self.last_heartbeats = {'Micro1':0,'Micro2':0,'Micro3':0,'Micro4':0}
        self.micro_statekey  = {'Micro1':'u1',
                                'Micro2':'u2',
                                'Micro3':'u3',
                                'Micro4':'u4',}

    def Generator(self,state,data,order):
        """ Updates state from micro data, at ~1,000,000 key,value pairs/sec """

        if data:

            micro_data, micro_times = data # 0 is data, 1 is receive times

            for m,data in enumerate(micro_data):

                if data != '-1': # do before to prevent KeyError
                    # Micro data elements take shape: key:value,key:value,...
                    data = data.split(',')
                    data.pop(-1) # always a '' at the end, because of trailing commas

                    data = (item.split(':') for item in data)

                    for key_value in data:
                        try:
                            key,value = key_value
                            if key in state:
                                state[key] = float(value)
                        except Exception as e:
                            print('State Generator ERROR:',e,
                                    '\n\tkey_val:',key_value)
                            continue

                    """ Assign 'generated' states (those not explicitly given) """

                    # Count status & delta, adjusted for Microlink recv times
                    if order[m]=='Micro1':

                        time_adj = sum([t for t in micro_times[ order.index('Micro2') : order.index('Micro1') ]])

                        state['C1']   = state['c1']+state['c1_']/1000

                    elif order[m]=='Micro2':

                        time_adj = sum([t for t in micro_times[ order.index('Micro1') : order.index('Micro2') ]])

                        state['C2']   = state['c2']+state['c2_']/1000

                    state['C1-2'] = state['C1'] - state['C2']

                    # Connection status based on heartbeats
                    if self.last_heartbeats[order[m]] != state['H']:
                        state[ self.micro_statekey[ order[m]] ] = 1
                        self.last_heartbeats[ order[m] ] = state['H']
                    else:
                        state[ self.micro_statekey[ order[m]] ] = 0

                else:
                    state[ self.micro_statekey[ order[m]] ] = 0

    def Telemetry(self,state,connections,verbose=False):
        for connection in connections:
            connection.update(state,verbose=verbose)

if __name__ == '__main__':
    # Test the generator with fake data

    from time import time

    size = 100000

    fake = {str(n):n for n in range(0,size)}
    fake_new = {str(n):n*size for n in range(0,size)}
    fake_micro = ''.join(['%s:%s,' % (key, int(value)) for (key, value) in fake_new.items()])

    state = State_Generator()

    start = time()
    state.Generator(fake,fake_micro)
    print((time()-start)*1000,'msec')
