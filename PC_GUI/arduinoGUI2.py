from audioop import findfit
from doctest import master
from glob import glob
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
from PIL import ImageTk, Image

#From arduinoCheckForData.py
from msilib import Table
import threading
import time
import datetime
from xmlrpc.client import DateTime
import numpy as np
import arduinoCommsB as aC
from pandas import read_csv			#May be able to delete this

#Import custom Global Variables
import GlobalVars as GV
debug = GV.debug
col_header = GV.col_header
row_header = GV.row_header
num_rows = GV.num_rows

user_comment = GV.user_comment


#TODO:
#PC GUI software updates: 
# 1) User Modifiable channel headings ✓ and Oven Names.
# 2) Data graphing.
# 3) User Comment field in data file.						✓
# 4) Data file naming										✓ 
# 5) Defult file-save directory saved between restarts.		✓
# 6) Udate HW settings from PC GUI
# 7) Add user comment field to each row/ Column
# 8) Why can't rows_headers be edited for rows above 10?  Becasue its a hexadecimal value lol.



#=========================
#System Defults:

config_location = Path('C:\JCS\OvenMonitorTime\param_config.ini')
sample_count = 0     #counts each time the machine is ran.  Used for incrementing file names
Path(GV.dir_name).mkdir(parents=True, exist_ok=True)     # set the config file location

#Defualt information saved in the param_config.ini file.  If the .ini is missing or
#Not written correctly, the old one will be overwritten with these values
file_name = 'TestData'      
test_dur = 2                            #Test Durration in seconds
sample_frequency = 100                  #Sample Frequency in Hz


#TODO
#Add Sample count to .csv file

#=========================
#  code to ensure a clean exit

def exit_handler():
    print ('My application is ending!')
    stopListening()
    aC.closeSerial()

atexit.register(exit_handler)

#===========================
#Global variables for this module

tkArd = tk.Tk()
tkArd.minsize(width=320, height=170)
tkArd.config()
tkArd.title("Oven Monitoring and Timing")

#Initialize the error message that appears at the bottom of the GUI
error_msg = StringVar() # for use in the mainscreen
#error_msg.set("starting")

# the next line must come after  tkArd = Tk() so that a StringVar()
#   can be created in checkForData.
#import arduinoCheckForData as cD # so we can refer to its variables


#======================
# get first view ready

def setupView():
	global masterframe
	masterframe = tk.Frame()
	masterframe.pack()
	
	selectPort()

#Read Configuration File
def read_config():
	global file_name, test_dur, sample_frequency, col_header, row_header, config_location
	config = configparser.RawConfigParser()
	print(config_location)
	try:      #check if a config file exists & has the correct sections.
		config.read(config_location)
		GV.dir_name = config.get('File', 'dir')
		file_name = config.get('File', 'file_name')
		col_header[0] =	config.get('Parameters', 'col_header0')
		col_header[1] =	config.get('Parameters', 'col_header1')
		col_header[2] =	config.get('Parameters', 'col_header2')
		col_header[3] =	config.get('Parameters', 'col_header3')
		col_header[4] =	config.get('Parameters', 'col_header4')
		col_header[5] =	config.get('Parameters', 'col_header5')
		col_header[6] =	config.get('Parameters', 'col_header6')
		col_header[7] =	config.get('Parameters', 'col_header7')

		for row in range(0,num_rows):
			option_text = "row_header" + str(row)		#Create a string for the config file identiier
			row_header[row] = config.get('Parameters', option_text)		#Read the config file
	
		if debug: print("Successfully read param_config.ini")
	except:
		#If config file has errors or DNE
		config.clear()           #Delete the existing param_config.ini
		config.add_section('File')
		config.set('File', 'dir', GV.dir_name)
		config.set('File', 'file_name', file_name)
		config.add_section('Parameters')
		config.set('Parameters', 'col_header0',col_header[0])
		config.set('Parameters', 'col_header1',col_header[1])
		config.set('Parameters', 'col_header2',col_header[2])
		config.set('Parameters', 'col_header3',col_header[3])
		config.set('Parameters', 'col_header4',col_header[4])
		config.set('Parameters', 'col_header5',col_header[5])
		config.set('Parameters', 'col_header6',col_header[6])
		config.set('Parameters', 'col_header7',col_header[7])

		for row in range(0,num_rows):
			option_text = "row_header" + str(row)		#Create a string for the config file identiier
			config.set('Parameters', option_text, row_header[row])		#Read the config file
		

		config.write(open(config_location, "w"))
		if debug: print("Deleted old and creating new param_config.ini file")



#Save Configuration
#Pulls values directly from tkinter GUI
def save_config():  
	if debug:print("Updating Config File")
	global error_msg, entry_file_name, file_name,  sample_count, config, user_comment, num_rows


	file_name = entry_file_name.get()
	user_comment = entry_user_comment.get()
	file_name = entry_file_name.get()
	

	config = configparser.RawConfigParser()
	if GV.dir_name==0:     #if a new directory is not defined, use the old
		config.read(config_location)
		GV.dir_name = config.get('File', 'dir')
	else:
		config.add_section('File')
		config.set('File', 'dir', GV.dir_name)
		config.set('File', 'file_name', file_name)

	if config.has_section('Parameters'):
		config.set('Parameters', 'col_header0',col_header[0])
		config.set('Parameters', 'col_header1',col_header[1])
		config.set('Parameters', 'col_header2',col_header[2])
		config.set('Parameters', 'col_header3',col_header[3])
		config.set('Parameters', 'col_header4',col_header[4])
		config.set('Parameters', 'col_header5',col_header[5])
		config.set('Parameters', 'col_header6',col_header[6])
		config.set('Parameters', 'col_header7',col_header[7])

		# config.set('Parameters', 'row_header0',row_header[0])
		# config.set('Parameters', 'row_header1',row_header[1])
		# config.set('Parameters', 'row_header2',row_header[2])
		# config.set('Parameters', 'row_header3',row_header[3])
		# config.set('Parameters', 'row_header4',row_header[4])
		# config.set('Parameters', 'row_header5',row_header[5])
		# config.set('Parameters', 'row_header6',row_header[6])
		# config.set('Parameters', 'row_header7',row_header[7])
		# config.set('Parameters', 'row_header8',row_header[8])
		# config.set('Parameters', 'row_header9',row_header[9])
		# config.set('Parameters', 'row_header10',row_header[10])
		# config.set('Parameters', 'row_header11',row_header[11])

		for row in range(0,num_rows):
			option_text = "row_header" + str(row)
			config.set('Parameters', option_text, row_header[row])
		
		#config.set('Parameters', 'test_dur', tk_test_dur.get())
		#config.set('Parameters', 'sample_frequency', tk_sample_frequency.get())
	else:
		config.add_section('Parameters')
		config.set('Parameters', 'col_header0',col_header[0])
		config.set('Parameters', 'col_header1',col_header[1])
		config.set('Parameters', 'col_header2',col_header[2])
		config.set('Parameters', 'col_header3',col_header[3])
		config.set('Parameters', 'col_header4',col_header[4])
		config.set('Parameters', 'col_header5',col_header[5])
		config.set('Parameters', 'col_header6',col_header[6])
		config.set('Parameters', 'col_header7',col_header[7])

		# config.set('Parameters', 'row_header0',row_header[0])
		# config.set('Parameters', 'row_header1',row_header[1])
		# config.set('Parameters', 'row_header2',row_header[2])
		# config.set('Parameters', 'row_header3',row_header[3])
		# config.set('Parameters', 'row_header4',row_header[4])
		# config.set('Parameters', 'row_header5',row_header[5])
		# config.set('Parameters', 'row_header6',row_header[6])
		# config.set('Parameters', 'row_header7',row_header[7])
		# config.set('Parameters', 'row_header8',row_header[8])
		# config.set('Parameters', 'row_header9',row_header[9])
		# config.set('Parameters', 'row_header10',row_header[10])

		for row in range(0,num_rows):
			option_text = "row_header" + str(row)
			config.set('Parameters', option_text, row_header[row])
				
		
		
		#config.set('Parameters', 'test_dur', tk_test_dur.get())
		#config.set('Parameters', 'sample_frequency', tk_sample_frequency.get())

	config.write(open(config_location, "w"))
	if debug:
		print("Config File Updated")
		print(f"Saving configuration for file: {file_name}")
		print(f"user_comment: {user_comment}")
		print(test_dur+1-1)  #A creative way to make sure we are working with ints.

	error_msg.set("Config File Updated")



	if debug == 2:
		SaveData(0)		#Create the datafile, save the headers but not any data.
	#    root.update_idletasks


# button to ask for directory to save output data to.
def get_dir():
	print("Selecting Directory")
	#global GV.dir_name
	GV.dir_name = filedialog.askdirectory(initialdir = "/",title = "Select Directory")
	dir_name_text.set(GV.dir_name)
	masterframe.update_idletasks()
	if GV.dir_name=='':
		error_msg.set("WARNING: No File Save Directory set")
	else:
		error_msg.set("Directory Updated")
	save_config()


		
#======================
# definition of screen to choose the Serial Port
def selectPort():
	global masterframe, radioVar
	for child in masterframe.winfo_children():
		child.destroy()
	radioVar = tk.StringVar()

	lst = aC.listSerialPorts()
	
	l1= tk.Label(masterframe, text = "Select Serial Port: ", width = 50, height = 2) 
	l1.pack()

	if lst[0]:
		for n in lst:
			r1 = tk.Radiobutton(masterframe, text=n, variable=radioVar, value=n)
			r1.config(command = radioBtnPress)
			r1.pack() # python 2.x use WEST
	else:
		l2 = tk.Label(masterframe, text = "No Serial Port Found")
		l2.pack(anchor=W)
		l3 = tk.Radiobutton(masterframe, text="OK", variable=radioVar, value=1)
		l3.config(command = mainScreen)
		l3.pack(anchor=W)




#======================
# definition of main screen to control Arduino
def mainScreen():
	global masterframe, dir_name_text, error_msg, entry_file_name, entry_user_comment
	for child in masterframe.winfo_children():
		child.destroy()
		
	tkArd.geometry("1000x600")		#Set the Width and Height of the mainScreen

	#Directory name label
	################################################
	read_config()
	dir_name_text = StringVar()
	dir_name_text.set(GV.dir_name)
	dir_name_label = tk.Label(masterframe, textvariable = dir_name_text)
	
	dir_name_text.set(GV.dir_name)
	masterframe.update_idletasks()
	################################################


	browse_dir_button = tk.Button(masterframe, text="Change File Save Directory", command=get_dir, bg='green', fg='white', font=('helvetica', 12, 'bold'))

	#data file name
	################################################
	file_name_label = tk.Label(masterframe, text="File Name:", justify=RIGHT,anchor=E)
	entry_file_name = tk.Entry(masterframe, bg='white', textvariable=file_name)
	#entry_file_name.insert(0, "Enter file name here...")
	entry_file_name.insert(0, file_name)

	################################################
	label_user_comment = tk.Label(masterframe, text="User Comment: ")
	entry_user_comment = tk.Entry(masterframe, bg='white', textvariable=user_comment)
	################################################s
	
	save_config_button = tk.Button(masterframe, text="Save current Configuration", command=save_config, bg='green', fg='white', font=('helvetica', 12, 'bold'))
	
	JCS_com_button= tk.Button(masterframe, text="Jake's Custom Shop, LLC", command=open_JCS_com, bd=0, fg='black', font=('helvetica', 10, 'underline'), justify='left')
	support_button = tk.Button(masterframe, text="Support", command=open_support_link, bd=0, fg='black', font=('helvetica', 10, 'underline'), justify='right')

	
	#File save dir

	SpacerA = tk.Label(masterframe, width = 5, height = 2) 
	SpacerB = tk.Label(masterframe, width = 5, height = 2) 
	SpacerC = tk.Label(masterframe, width = 5, height = 2) 
	SpacerD = tk.Label(masterframe, width = 5, height = 2) 
	error_label = tk.Label(masterframe, textvariable = error_msg) 
	
	 
	SpacerA.grid(row = 0, column = 5, columnspan=5)
	dir_name_label.grid(row = 1, column = 1, columnspan=1)
	#SpacerB.grid(row = 2, column = 0, columnspan=5)
	browse_dir_button.grid(row=1, column=2, columnspan=1)
	SpacerC.grid(row = 4, column = 0, columnspan=5)
	label_user_comment.grid(row = 5, column=1,columnspan=1)
	entry_user_comment.grid(row=5, column=2, columnspan=1)
	SpacerD.grid(row = 6, column = 0, columnspan=5)
	file_name_label.grid(row = 7, column = 1, columnspan = 1)
	entry_file_name.grid(row = 7, column = 2, columnspan = 1)
	save_config_button.grid(row = 8, column = 1, columnspan=2)
	#SpacerC.grid(row = 5, column = 0, columnspan=5)


	build_table(masterframe)
	error_label.grid(row = 10, column = 0, columnspan=5)
	JCS_com_button.grid(row=11, column=0, columnspan=2)
	support_button.grid(row=11, column=2, columnspan=2)



def open_support_link():
    import webbrowser
    webbrowser.open("https://docs.google.com/document/d/1gjY1_6nVbyH6iNai8lv_TL_Sg71yOVd7sbvwQCloG9I/edit?usp=sharing")
def open_JCS_com():
    import webbrowser
    webbrowser.open("https://jakescustomshop.com")




	
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
	listenForData()
	mainScreen()




#============================
# Create a treeview table

def build_table(masterframe):


	global table, col_header, row_header
	table = ttk.Treeview( 
		masterframe, 
		columns= ('ovenid','count', 'temp1','temp2','temp3','temp4','temp5','temp6','temp7','temp8', 'status'),
		
		show='headings')

	#Table row defined here
	table.grid(row=num_rows,column=1,columnspan = 2, ipadx=10, ipady=num_rows*3)	#Required to show table in masterframe
	
	#build the table columns
	table.column('ovenid', width=75)
	table.heading('ovenid', text='Oven ID')
	table.column('count', width=75)
	table.heading('count', text='Sample #')
	table.column('status', width=50)
	table.heading('status', text='Status')

	# Bind selection event to the treeview
	table.bind("<ButtonRelease-1>", row_header_callback)


	for num in range(1,9):
		table.column('temp{}'.format(num),width=75)
		table.heading('temp{}'.format(num), text=col_header[num-1], command=lambda c=num: col_header_callback(c-1))
		print(col_header[num-1])

	# Insert some data into the table
	for num in range(0,num_rows):
		table.insert('','end', values=(row_header[num], ' ') , iid=num, tags=('ttk' ,'simple'))
		# table.tag_configure('ttk', background='lightgray')




#============================
# Ask the user to name a row header
def row_header_callback(event):
	global row_header, table
	print(row_header)
	item = table.identify_region(event.x, event.y)
	col = table.identify_column(event.x)
	row = table.identify_row(event.y)
	print(f"row: {row}")
	row = int(row)

	

	if (item == "cell"):
		print(f"Cell selected - Column: {col}, Row: {row}")
	if (col == "#1"):
		print(f"Row {row} Selected")
		
		edit_row_header_win = Tk()
		edit_row_header_win.minsize(width=320, height=170)
		edit_row_header_win.title("Enter Row Name")
		# Label asking for user input
		label = Label(edit_row_header_win, text="Enter the row header name:")
		label.pack()

		# Entry field for user input
		entry_row_name = Entry(edit_row_header_win)
		entry_row_name.pack()

		# Callback function for when a user Submits a new row header name.
		def get_name_and_close():
			#This function retrieves the text from the entry field and closes the window.
			row_header[row] = entry_row_name.get()
			
			if row_header[row]:  # Check if user entered a name
				# Use the row_name variable in your program logic
				print(f"You entered: {row_header[row]}")
				edit_row_header_win.destroy()  # Close the windows
				table.item(table.get_children()[row], values=row_header[row])
				save_config()
			# else:
				# print("Please enter a row header name.")

		# Button to submit the name
		button = Button(edit_row_header_win, text="Submit", command=get_name_and_close)
		button.pack()


		edit_row_header_win.mainloop()




#======================
#ask the user to specify a column name
def col_header_callback(num):
	print(f"Collumn {num} Selected")
	global col_header, table
	
	edit_col_header_win = Tk()
	edit_col_header_win.minsize(width=320, height=170)
	edit_col_header_win.title("Enter Column Header Name")
	# Label asking for user input
	label = Label(edit_col_header_win, text="Enter the column header name:")
	label.pack()

	# Entry field for user input
	entry_column_name = Entry(edit_col_header_win)
	entry_column_name.pack()


	# Callback function for when a user Submits a new column header name.
	def get_name_and_close():
		#This function retrieves the text from the entry field and closes the window.
		col_header[num] = entry_column_name.get()
		if col_header[num]:  # Check if user entered a name
			# Use the column_name variable in your program logic
			print(f"You entered: {col_header[num]}")
			edit_col_header_win.destroy()  # Close the windows
			table.destroy()
			build_table(masterframe)
			save_config()
		else:
			print("Please enter a column header name.")

	# Button to submit the name
	button = Button(edit_col_header_win, text="Submit", command=get_name_and_close)
	button.pack()


	edit_col_header_win.mainloop()

	




#======================

# global variables for module

inputData = ""
threadRun = True
checkDelay = 0.2 # seconds

displayVal = StringVar() # for use in the mainscreen
displayVal.set("starting")

#======================

class DataClass():
	def __init__(self):
		self.dateTime = []			#Day-HR-Min-Sec for each data transmission
		self.Temps = []				#List of Lists
		self.OvenID: int			
		self.Count = []				#Counter for transmitted data.  generated by Transmitter
		self.PrevStatus = 0			#The previous oven status.  Used to determine a state change
		self.Status = [0]			#List of oven statuses.  Current status is self.Status[-1]
	
	#Append data to the relavent lists
	def append_Temps(self, data):
		self.Temps.append(data)
	def append_Count(self, data):
		self.Count.append(data)
	def append_dateTime(self, data):
		self.dateTime.append(data)
	def append_Status(self, data):
		self.Status.append(data)

	#Converts class data to a comma seperated string.  (does not write to a .csv)
	def to_csv_string(self):
		csv_string = ""
		for i in range(len(self.dateTime)):
			csv_string += f"{self.dateTime[i]},"
			csv_string += f"{self.Count[i]},"
			for j in range(8):
				csv_string += f"{self.Temps[i][j]},"
			csv_string += f"{system_status_list[self.Status[i]]}\n"
		return csv_string

#An array of class objects.  First element is empty so indexing can be done with OvenID number (which starts at 1)
OvenDataObject = [0, DataClass(),DataClass(),DataClass(),DataClass(),DataClass(),DataClass(),DataClass(),DataClass()]



def checkForData():
	global threadRun, checkDelay,  error_msg
	print ("Starting to Listen")
	oldDataInput = "waiting"
	while threadRun == True:
		dataInput = aC.recvFromArduino(0.1)
		dataInput = np.fromstring(dataInput, dtype=int, sep=',')
		print(dataInput)
		if str(dataInput) == "<<" or str(dataInput) == ">>":
			dataInput = "nothing"
			print ("DataInput %s" %(dataInput))
		if (dataInput.any()):
			processData(dataInput)
			update_row(table,dataInput[0]-1, dataInput)		#Update the table.  col 0 is the row number 
		time.sleep(checkDelay)

		#Check each of the OvenDataObjects for a change to Acknowledged Status Byte.  Also check
		#and save data to a fresh .csv file.
		for i in range(1,len(OvenDataObject)):
			#Oven Status is updated to Acknowledge. We are not in Status 0 (Startup)
			if OvenDataObject[i].Status[-1] == 4 and OvenDataObject[i].PrevStatus !=4 and OvenDataObject[i].PrevStatus:
				SaveData(i)
				#OvenDataObject.Status = 5						#Temperature Data file saved
			OvenDataObject[i].PrevStatus = OvenDataObject[i].Status[-1]
				
	print("Finished Listening")
	print(OvenDataObject[2].Temps)


#Saves a 2D array to a .csv file from Global OvenDataObject
def SaveData(OvenID):
	global user_comment, OvenDataObject

	#File Saving Stuff
	#===========================
	file_name = buildFileName(OvenID)
	dir_filename = GV.dir_name + '/' + file_name + '.csv'
	if debug:print(dir_filename)
	
	try:        #Prevent the user from acidently overwriting old data.
		df = read_csv(dir_filename)
		error_msg = "File already exists, Please use a different file name"
		return
	except:
		0

	if debug:print("dir_filename: " + dir_filename)
	if debug:print("user_comment: " + user_comment)
		
	with open(dir_filename, 'x') as f:          #Creates a file if none exists.  Returns error if it does. 
		if debug:print('Writing data to ' + dir_filename, end='')

		# Write a header to the file
		f.write("OvenID: " + str(OvenID) + "\n")
		f.write("User Comment: " + str(user_comment) + "\n")
		f.write("Date Time, ");
		f.write("Sample #, ");
		for chan_num in range(8):
			f.write(col_header[chan_num]+', ')
		f.write("Status, ");
		f.write(u'\n')
		if debug==1:print(len(OvenDataObject[OvenID].Temps))

		
		#Saves the data to a .csv for the oven who's cook time is complete
		f.write(OvenDataObject[OvenID].to_csv_string())

	#Clear the arrays that are appened
	#System currently records all data up until the status is changed to "Acknowledge" (4)
	OvenDataObject[OvenID].Temps = []
	OvenDataObject[OvenID].dateTime = []
	OvenDataObject[OvenID].Status = [0]

		


	#===========================

system_status_list = [
    "STARTUP",
    "OVEN_READY",
    "CYCLE_ACTIVE",
    "TIME_COMPLETE",
    "ACKNOWLEDGED",
    "TEMPERATURE_DATA_SAVED",
    "ERROR",
]






#======================
# takes an array of data and assigns to the OvenDataObject with a matching Oven ID
def processData(dataRecvd):
	global displayVal
	displayVal.set(dataRecvd)
	global OvenDataObject
	now = datetime.datetime.now()	# Get the current date and time
	OvenID = dataRecvd[0]	
	#Asign dataRecvd cells to appropriate locations in OvenDataObject
	OvenDataObject[OvenID].OvenID = OvenID
	OvenDataObject[OvenID].append_Count(dataRecvd[1])
	OvenDataObject[OvenID].append_Temps([dataRecvd[2], dataRecvd[3], dataRecvd[4], dataRecvd[5],dataRecvd[6], dataRecvd[7], dataRecvd[8], dataRecvd[9]])
	OvenDataObject[OvenID].append_dateTime(now.strftime("%Y-%m-%d %H:%M:%S"))
	OvenDataObject[OvenID].append_Status(dataRecvd[10])
	print("OvenDataObject[i].Status[-1]" + str(OvenDataObject[OvenID].Status[-1]))

	

#======================

def listenForData():
	t = threading.Thread(target=checkForData)
	t.daemon = True
	t.start()

#======================

def stopListening():
	global threadRun, checkDelay
	threadRun = False
	time.sleep(checkDelay + 0.1) # allow Thread to detect end


#============================
#Update a specified row in a tkinter Tree View Table
def update_row(table, row_index, new_values):
	if (debug):
		print("Updating Table")
	new_values = list(new_values)
	new_values[0] = row_header[new_values[0]-1]		#replace the oven num (stored in new_values[0]) with row_header.  Subtract 1 from Oven number 
	table.item(table.get_children()[row_index], values=new_values)
	# table.item(table.get_children()[row_index-1], values=row_header[row_index])



def buildFileName(oven_id: int) -> str:
  """Builds a string with the OvenID number and today's date time in the format YYYYMMDD-HHMM
  Returns:
    A string with the OvenID number and today's date time in the format YYYYMMDD-HHMM.
  """
  global file_name

  date_time = datetime.datetime.today()
  date_time_str = date_time.strftime("%Y%m%d-%H%M%S")
  return f"{oven_id}-{file_name}-{date_time_str}"






#======================
# code to start the whole process

setupView()
tkArd.mainloop()







