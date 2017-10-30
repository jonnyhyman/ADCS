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

from multiprocessing import Process,Pipe
import xml.etree.ElementTree as ET
from time import time,sleep
import subprocess
import socket
import ast

class IP_Scan(object):

    """
    Determine a host's IP address given its MAC address and an IP address
    range to scan for it.

    I created this to discover a WLAN printer (which dynamically gets an IP
    address assigned via DHCP) on the local network.

    Calls Nmap_ to ping hosts and return their MAC addresses (requires root
    privileges).

    Requires Python_ 2.7+ or 3.3+.

    .. _Nmap: http://nmap.org/
    .. _Python: http://python.org/

    :Copyright: 2014-2016 `Jochen Kupperschmidt
    :Date: 27-Mar-2016 (original release: 25-Jan-2014)
    :License: MIT
    :Website: http://homework.nwsnet.de/releases/9577/#find-ip-address-for-mac-address
    """

    def scan_for_hosts(self,ip_range):
        """Scan the given IP address range using Nmap and return the result
        in XML format.
        """
        nmap_args = ['nmap', '-n', '-sP', '-oX', '-', ip_range]
        return subprocess.check_output(nmap_args)


    def find_ip_address_for_mac_address(self,xml, mac_address):
        """Parse Nmap's XML output, find the host element with the given
        MAC address, and return that host's IP address (or `None` if no
        match was found).
        """
        host_elems = ET.fromstring(xml).iter('host')
        host_elem = self.find_host_with_mac_address(host_elems, mac_address)
        if host_elem is not None:
            return self.find_ip_address(host_elem)


    def find_host_with_mac_address(self,host_elems, mac_address):
        """Return the first host element that contains the MAC address."""
        for host_elem in host_elems:
            if self.host_has_mac_address(host_elem, mac_address):
                return host_elem


    def host_has_mac_address(self,host_elem, wanted_mac_address):
        """Return true if the host has the given MAC address."""
        found_mac_address = self.find_mac_address(host_elem)
        return (
            found_mac_address is not None and
            found_mac_address.lower() == wanted_mac_address.lower()
        )


    def find_mac_address(self,host_elem):
        """Return the host's MAC address."""
        return self.find_address_of_type(host_elem, 'mac')


    def find_ip_address(self,host_elem):
        """Return the host's IP address."""
        return self.find_address_of_type(host_elem, 'ipv4')


    def find_address_of_type(self,host_elem, type_):
        """Return the host's address of the given type, or `None` if there
        is no address element of that type.
        """
        address_elem = host_elem.find('./address[@addrtype="{}"]'.format(type_))
        if address_elem is not None:
            return address_elem.get('addr')


    def Find(self,mac_address='6C:0B:84:CA:21:EA',ip_range = '192.168.2.1-255'):

        ip_address=None
        while not ip_address:

            print('    -> Searching for MAC address {} in IP address range {}.'
                  .format(mac_address, ip_range))

            xml = self.scan_for_hosts(ip_range)
            ip_address = self.find_ip_address_for_mac_address(xml, mac_address)

            if ip_address:
                print('    -> Found IP address {} for MAC address {} in IP address range {}.'
                      .format(ip_address, mac_address, ip_range))
            else:
                print('    -> No IP address found for MAC address {} in IP address range {}.'
                      .format(mac_address, ip_range))

        else:
            return ip_address

class Wireless_Sensing(object):
    def __init__(self,port,mac_addr=None,ip=None):
        """ Finds the wirless sensor's IP by its mac address """

        if ip is None:
            self.UDP_IP   = IP_Scan().Find(mac_address = mac_addr)
        elif mac_addr is None:
            self.UDP_IP = ip

        self.UDP_PORT = port


        self.sensor, self.link = Pipe()
        self.comm_prc = Process(target=self.sensor_check,args=(self.link,))
        self.comm_prc.daemon = True # quit when main quits
        self.comm_prc.start()

    def sensor_check(self,link):
        """ Connect to sensor and parse its data """

        sock = socket.socket(socket.AF_INET,    # Internet
                             socket.SOCK_DGRAM) # UDP

        while True:

            sock.settimeout(2)

            try:
                sock.sendto(bytes("t",'utf-8'), (self.UDP_IP, self.UDP_PORT))
            except socket.timeout:
                pass

            try:

                data,addr = sock.recvfrom(1024) # buffer size is 1024 bytes

                if data:

                    data = data.decode('utf-8')
                    data = data.replace("V0"," 'v0' ")
                    data = data.replace("P0"," 'p0' ")
                    data = data.replace("T0"," 't0' ")
                    data = data.replace("M0"," 'm0' ")
                    data = ''.join( ['{',data,'}'] )
                    data = ast.literal_eval(data) # make into actual dictionary
                    link.send(data)

            except socket.timeout:
                pass

    def update(self,state,verbose=False):
        """ Receive from the sensor_check process pipe; update state"""

        if self.sensor.poll():
            if verbose:
                print('>got poll')
            data = self.sensor.recv()
            if data:
                if verbose:
                    print('  >got data:',data)
                try:
                    state['v0'] = data['v0']
                    state['p0'] = data['p0']
                    state['t0'] = data['t0']
                    state['m0'] = data['m0']
                    if verbose:
                        print('      >got state upd:',state)
                except KeyError as e:
                    if verbose:
                        print('     >got KeyError',e)
                    pass # partial data will cause this
        else:
            if verbose:
                print('>no poll')

    def close(self):
        self.comm_prc.terminate()


if __name__ == '__main__':

    mac  = "6C:0B:84:CA:21:EA"
    port = 44013    # the molecular mass of N2O! :)
    n2o  = Wireless_Sensing(mac_addr=mac,port=port)

    state = {'v0':-1,'p0':-1,'t0':-1}

    while True:
        sleep(0.1)
        n2o.update(state)
        print(state)
