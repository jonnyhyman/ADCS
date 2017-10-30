from math import inf

# which state variables are not instrumentation
Control_Keys = ['A','B','F1','I','R',
                'u1','u2','adcs','hdcs',
                'E','a','F1','F2','B1','B2',
                'C','C_','Cm']

 # which control variables are actionable via adcs
Command_Keys = ['E','a','F1','F2','B1','B2','c1','c1_','c2','c2_','Cm',
                'TM','UC','RS']

# which keys should be sent together on Microlink (key teams)
#Key_Teams = {'Cm':('C','C_'),}

mac  = "6C:0B:84:CA:21:EA"
port = 44013    # the molecular mass of N2O! :)

# address finding data for wireless sensor telemetry
N2O = {'mac' : "6C:0B:84:CA:21:EA", 'port' : 44013, 'ip':'192.168.2.3'}

State = {
    'E'   :  0, # Emergency STOP
    'a'   :  0, # Arm Mode
    'F1'  :  0, # Fire Suppression 1
    'F2'  :  0, # Fire Suppression 2
    'c1'   :  0, # Count Timer 1
    'c1_'  :  0, # Count microseconds
    'c2'   :  0, # Count Timer 2
    'c2_'  :  0, # Count microseconds
    'Cm'  :  0, # Count Mode (Halt,Count,Set)
    'TM'  :  0, # Test matrix upload trigger
    'ER'  :  0, # Exceeded range trigger
    'UC'  :  0, # Microcontroller upload trigger
    'RS'  :  0, # ADCS / Microcontroller resetting trigger
    'B1'  :  0, # Bus 1 (Control to v1)
    'B2'  :  0, # Bus 2 (Control to v2)
    'g'   : -1,
    'z0'  : -1,
    'z1'  : -1,
    'z2'  : -1,
    'z3'  : -1,
    'v0'  : -1,
    'v1'  : -1,
    'v2'  : -1,
    'i1'  : -1,
    'i2'  : -1,
    'm0'  : -1,
    'p0'  : -1,
    'p1'  : -1,
    't0'  : -1,
    't1'  : -1,
    't2'  : -1,
    'dt'  : -1,
    'it'  : -1,
    'Fa1' :  0,
    'Fa2' :  0,
    'A'   :  0,
    'B'   :  0,
    'I'   :  0,
    'R'   :  0,
    'u1'  :  0,
    'u2'  :  0,
    'u3'  :  0,
    'u4'  :  0,
    'adcs':  0,
    'hdcs':  0,
    # Below here are ADCS "GENERATED"/calculated variables
    'H'   :  0, # constantly overriden per check
    'C1'  :  0, # u1 count
    'C2'  :  0, # u2 count
    'C1-2':  0, # c1-c2
}

Command={}
for key in State.keys():
    if key in Command_Keys:
        Command[key] = 0

State_Limits = {
    'g'   : [-5,5],
    'z0'  : [-500,500],
    'z1'  : [-500,500],
    'z2'  : [-500,500],
    'z3'  : [-500,500],
    'v0'  : [3.4,4.2],
    'v1'  : [11,13],
    'v2'  : [11,13],
    'i1'  : [-30, 30],
    'i2'  : [-15, 15],
    'm0'  : [-inf,inf],
    'p0'  : [-inf,17.3], # 0-2500psig
    'p1'  : [-inf,3548],
    't0'  : [20,50],
    't1'  : [-inf,300],
    't2'  : [-inf,300],
    'dt'  : [-inf, 1],
    'it'  : [-5,50],
    'C1-2': [-inf,0.5],
}

State_Limits_keys = State_Limits.keys()
