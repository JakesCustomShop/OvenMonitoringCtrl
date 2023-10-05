from audioop import findfit
from doctest import master
import tkinter as tk
from tkinter import *
from tkinter import ttk
from tkinter import END, filedialog
from tkinter.tix import COLUMN  
from turtle import width
import arduinoCommsB as aC
import atexit
import numpy as np
import configparser
from pathlib import Path


################################################
global debug
#debug = 0   #Debug mode off.
debug = 1   #Print Statments
#debug = 2  #skips datacollection


################################################
#System Defults:

Path('C:\JCS\OvenMonitorCtrl').mkdir(parents=True, exist_ok=True)     # set the config file location
config_location = Path('C:\JCS\OvenMonitorCtrl\param_config.ini')
calibration_csv = "C:\JCS\OvenMonitorCtrl\calibration.csv"
sample_count = 0     #counts each time the machine is ran.  Used for incrementing file names

#Defualt information saved in the param_config.ini file.  If the .ini is missing or
#Not written correctly, the old one will be overwritten with these values.
dir_name = "C:\JCS\OvenMonitorCtr"   #Save location for .csv data.
file_name = 'TestData'      
test_dur = 2                            #Test Durration in seconds
sample_frequency = 100                  #Sample Frequency in Hz

################################################


#=========================
#  code to ensure a cleaan exit

def exit_handler():
    print ('My application is ending!')
    cD.stopListening()
    aC.closeSerial()

atexit.register(exit_handler)

#===========================
# global variables for this module

tkArd = tk.Tk()
tkArd.minsize(width=320, height=170)
tkArd.config()
tkArd.title("Oven Monitoring and Timing")

	# the next line must come after  tkArd = Tk() so that a StringVar()
	#   can be created in checkForData.
import arduinoCheckForData as cD # so we can refer to its variables

#======================
# get first view ready

def setupView():
	global masterframe
	masterframe = tk.Frame()
	masterframe.pack()
	
	selectPort()

#Read Configuration File
def read_config():
    global dir_name, file_name, test_dur, sample_frequency

    config = configparser.RawConfigParser()
    try:      #check if a config file exists & has the correct sections.
        config.read(config_location)
        dir_name = config.get('File', 'dir')
        file_name = config.get('Parameters', 'file_name')
        test_dur = config.getfloat('Parameters', 'test_dur')
        sample_frequency = config.getfloat('Parameters', 'sample_frequency')
        if debug: print("Successfully read param_config.ini")

    except: #If config file has errors or DNE
        config.clear()           #Delete the existing param_config.ini
        config.add_section('File')
        config.set('File', 'dir', dir_name)
        config.add_section('Parameters')
        config.set('Parameters', 'file_name', file_name)
        config.set('Parameters', 'test_dur', test_dur)
        config.set('Parameters', 'sample_frequency', sample_frequency)
        config.write(open(config_location, "w"))
        if debug: print("Deleted old and creating new param_config.ini file")


# button to ask for directory to save output data to.
def get_dir():
    global dir_name
    dir_name = filedialog.askdirectory(initialdir = "/",title = "Select Directory")
    dir_name_text.set(dir_name)
    masterframe.update_idletasks()


		
#======================
# definition of screen to choose the Serial Port
def selectPort():
	global masterframe, radioVar
	for child in masterframe.winfo_children():
		child.destroy()
	radioVar = tk.StringVar()

	lst = aC.listSerialPorts()
	
	l1= tk.Label(masterframe, width = 5, height = 2) 
	l1.pack()

	if len(lst) > 0:
		for n in lst:
			r1 = tk.Radiobutton(masterframe, text=n, variable=radioVar, value=n)
			r1.config(command = radioBtnPress)
			r1.pack(anchor=W) # python 2.x use WEST
	else:
		l2 = tk.Label(masterframe, text = "No Serial Port Found")
		l2.pack()




#======================
# definition of main screen to control Arduino
def mainScreen():
	global masterframe, dir_name_text
	for child in masterframe.winfo_children():
		child.destroy()
		
	tkArd.geometry("600x500")

	#Directory name label
	################################################
	read_config()
	dir_name_text = StringVar()
	dir_name_text.set(dir_name)
	dir_name_label = tk.Label(masterframe, textvariable = dir_name_text)
	
	dir_name_text.set(dir_name)
	masterframe.update_idletasks()
	################################################


	browse_dir_button = tk.Button(masterframe, text="Change File Save Directory", command=get_dir, bg='green', fg='white', font=('helvetica', 12, 'bold'))

	testButton = tk.Button(masterframe, text="Test Button")

	#File save dir

	SpacerA = tk.Label(masterframe, width = 5, height = 2) 
	SpacerB = tk.Label(masterframe, width = 5, height = 2) 
	SpacerC = tk.Label(masterframe, width = 5, height = 2) 
	labelE = tk.Label(masterframe, textvariable = cD.displayVal) 
	
	 
	SpacerA.grid(row = 0, column = 0, columnspan=5)
	dir_name_label.grid(row = 1, column = 0, columnspan=5)
	SpacerB.grid(row = 2, column = 0, columnspan=5)
	browse_dir_button.grid(row=3, column=1, columnspan=2)
	SpacerB.grid(row = 4, column = 0, columnspan=5)

	cD.build_table(masterframe)




	
#=========================
# various callback functions
	
def btnA(btn):
	global ledAstatus, ledBstatus, servoPos
	
	if ledAstatus == 0:
		ledAstatus = 1
		btn.config(bg="white", fg="black")
	else:
		ledAstatus = 0
		btn.config(fg="white", bg="black")
	aC.valToArduino(ledAstatus, ledBstatus, servoPos)

def btnB(btn):
	global ledAstatus, ledBstatus, servoPos
	
	if ledBstatus == 0:
		ledBstatus = 1
		btn.config(bg="white", fg="black")
	else:
		ledBstatus = 0
		btn.config(fg="white", bg="black")
	aC.valToArduino(ledAstatus, ledBstatus, servoPos)


def slide(sval):
	global ledAstatus, ledBstatus, servoPos
	
	servoPos = sval
	aC.valToArduino(ledAstatus, ledBstatus, servoPos)

def radioBtnPress():
	global radioVar
	aC.setupSerial(radioVar.get())
	cD.listenForData()
	mainScreen()


#======================
# code to start the whole process

setupView()
tkArd.mainloop()




