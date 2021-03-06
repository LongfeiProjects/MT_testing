# Author(s): Djordje Vukcevic
# Year: 2019
import sys, os
import numpy as np
import sympy as sp
from sympy import *
import matplotlib.pyplot as plt
import pyinotify

desired_dim = np.int(sys.argv[1])

print("Selected dimension: ", desired_dim)
variable_num = 7

filename = "../archive/control_error.txt"

def restart_program(): #restart application
    python = sys.executable
    os.execl(python, python, * sys.argv)

class ModHandler(pyinotify.ProcessEvent):
    # evt has useful properties, including pathname
    def process_IN_CLOSE_WRITE(self, evt):
        print("Data file has changed")
        restart_program()


handler = ModHandler()
wm = pyinotify.WatchManager()
notifier = pyinotify.Notifier(wm, handler)
wdd = wm.add_watch(filename, pyinotify.IN_CLOSE_WRITE)

with open(filename, "r") as f:
    all_data = [x.split() for x in f.readlines()]
    input_data = np.array(all_data)[:-2]

rows = np.shape(input_data)[0] - 2 
cols = np.shape(input_data[0])[0]
rows = rows - (rows % variable_num)
num_samples = np.int(rows / variable_num) 
print("Data size: ", num_samples, ",", cols)

measured  = []
desired   = []
raw_error = []
error     = []
bias      = []
gain      = []
command   = []
contact_time_tick = []

for sample_ in range (2, rows + 2, variable_num):
    tick = np.float32( input_data[sample_][11] )
    if (tick > 0): contact_time_tick.append(tick)

    measured.append(    np.float32( input_data[    sample_][desired_dim]) )
    desired.append(     np.float32( input_data[1 + sample_][desired_dim]) )
    if (desired_dim == 6):
        raw_error.append(   np.float32( input_data[2 + sample_][6] ) )
        error.append(       np.float32( input_data[3 + sample_][0] ) )
        bias.append(        np.float32( input_data[4 + sample_][0] ) )
        gain.append(        np.float32( input_data[5 + sample_][0] ) )
        command.append(     np.float32( input_data[6 + sample_][0] ) )

    elif (desired_dim == 7):
        raw_error.append(   np.float32( input_data[2 + sample_][5] ) )
        error.append(       np.float32( input_data[3 + sample_][5] ) )
        bias.append(        np.float32( input_data[4 + sample_][5] ) )
        gain.append(        np.float32( input_data[5 + sample_][5] ) )
        command.append(     np.float32( input_data[6 + sample_][5] ) )
    
    elif (desired_dim == 8):
        raw_error.append(   np.float32( input_data[2 + sample_][2] ) )
        error.append(       np.float32( input_data[3 + sample_][2] ) )
        bias.append(        np.float32( input_data[4 + sample_][2] ) )
        gain.append(        np.float32( input_data[5 + sample_][2] ) )
        command.append(     np.float32( input_data[6 + sample_][2] ) )

    elif (desired_dim == 9):
        raw_error.append(   np.float32( input_data[2 + sample_][3] ) )
        error.append(       np.float32( input_data[3 + sample_][3] ) )
        bias.append(        np.float32( input_data[4 + sample_][3] ) )
        gain.append(        np.float32( input_data[5 + sample_][3] ) )
        command.append(     np.float32( input_data[6 + sample_][3] ) )

    elif (desired_dim == 10):
        raw_error.append(   np.float32( input_data[2 + sample_][4] ) )
        error.append(       np.float32( input_data[3 + sample_][4] ) )
        bias.append(        np.float32( input_data[4 + sample_][4] ) )
        gain.append(        np.float32( input_data[5 + sample_][4] ) )
        command.append(     np.float32( input_data[6 + sample_][4] ) )
 
    else:
        raw_error.append(   np.float32( input_data[2 + sample_][desired_dim] ) )
        error.append(       np.float32( input_data[3 + sample_][desired_dim] ) )
        bias.append(        np.float32( input_data[4 + sample_][desired_dim] ) )
        gain.append(        np.float32( input_data[5 + sample_][desired_dim] ) )
        command.append(     np.float32( input_data[6 + sample_][desired_dim] ) )

samples   = np.arange(0, num_samples, 1)
measured  = np.array(measured)
desired   = np.array(desired)
raw_error = np.array(raw_error)
error     = np.array(error) 
bias      = np.array(bias)
gain      = np.array(gain)
command   = np.array(command)

# f = open("guru99.txt","w+")
# for i in range(num_samples):
#     f.write("%f\n" % bias[i])
# f.close() 

plt.ion()
plt.figure(figsize = (15, 5))
if(desired_dim is 0):   plt.suptitle('Linear X', fontsize=20)
elif(desired_dim is 1): plt.suptitle('Linear Y', fontsize=20)
elif(desired_dim is 2): plt.suptitle('Linear Z', fontsize=20)
elif(desired_dim is 3): plt.suptitle('Angular X', fontsize=20)
elif(desired_dim is 4): plt.suptitle('Angular Y', fontsize=20)
elif(desired_dim is 5): plt.suptitle('Angular Z', fontsize=20)
elif(desired_dim is 6): plt.suptitle('Linear Velocity X', fontsize=20)
elif(desired_dim is 7): plt.suptitle('Angular Velocity Z', fontsize=20)
elif(desired_dim is 8): plt.suptitle('Linear Force Z', fontsize=20)
elif(desired_dim is 9): plt.suptitle('Angular Force X', fontsize=20)
elif(desired_dim is 10): plt.suptitle('Angular Force Y', fontsize=20)

tick_freq = 1000
if (num_samples > 4000 and num_samples < 7000): tick_freq = 500
elif (num_samples < 4000 and num_samples > 1000): tick_freq = 200
elif (num_samples < 1000 and num_samples > 500): tick_freq = 100
elif (num_samples < 500): tick_freq = 50

# plt.gca().set_axis_off()
plt.subplots_adjust(hspace = 0.02, wspace = 15)
plt.subplots_adjust(left=0.05, right=0.99, top=0.9, bottom=0.1)
plt.margins(0,0)

if (desired_dim > 5):
    max_command = np.float32( input_data[1][0] )
else:
    max_command = np.float32( input_data[1][desired_dim] )

plt.plot(bias * max_command, c = 'green', label='bias', linewidth = 2, zorder = 4)
plt.plot(gain * max_command, c = 'red', label=r'gain * sign(e)', linewidth = 2, zorder = 2)
plt.plot(command * max_command, c = 'blue', label='u', linewidth = 1.0, zorder = 3)
for tick in contact_time_tick:
    plt.axvline(x = tick, linewidth = 1.3,  color='blue', zorder = 1)
plt.yticks(np.arange(-max_command - 5 , max_command + 5, 10))

plt.legend(fontsize = 'x-large')
plt.xticks(np.arange(0, num_samples, tick_freq))
plt.grid(True)

plt.draw()
plt.pause(0.001)

notifier.loop()