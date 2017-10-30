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

from time import time
import os

class Log(object):
    def __init__(self,data_dicts):
        """ This class operates extremely fast data logging to disk.

            - To be extremely fast, we hold the file pointer until we
              are completely done, at which point we close.

            - We also make use of join method rather than concatenate, and
              only perform 1 write per commit

        """

        self.data_dicts = data_dicts # take a list of pointers to data dicts
        self.start = time()

        logno=0
        while (os.path.isfile('./logs/'+"log_"+str(logno)+".csv")):
            logno+=1

        self.file = open('./logs/'+"log_"+str(logno)+".csv",'a')

        join_list = ['T'] # Time always goes first!

        for data in self.data_dicts: # Get keys in each data_dict
            join_list.extend([',%s' % key for key in data])

        join_list.extend(['\n'])

        self.file.write(''.join(join_list))

    def Commit(self):
        """ Commit the current data dictionaries to file """

        join_list = [str(time()-self.start)] # Time always goes first!

        for data in self.data_dicts: # Get data in each data_dict
            join_list.extend([',%s' % data[key] for key in data])

        join_list.extend(['\n'])

        self.file.write(''.join(join_list))

    def close(self):
        """ Close the log file """

        self.file.close()
