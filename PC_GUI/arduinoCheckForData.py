
from msilib import Table
from tkinter import *
from tkinter import ttk
import threading
import time
import numpy as np
import arduinoCommsB as aC
from pandas import read_csv
#from arduinoGUI2 import error_msg
#from arduinoGUI2 import debug

debug = 0	#No Debugging
#debug = 1	#Debuging messages.


#======================

# global variables for module

inputData = ""
threadRun = True
checkDelay = 0.2 # seconds

testVar = "testvar"

displayVal = StringVar() # for use in the mainscreen
displayVal.set("starting")

#======================

def checkForData():
	global threadRun, checkDelay, dir_name, error_msg
	print ("Starting to Listen")
	oldDataInput = "waiting"
	while threadRun == True:
		dataInput = aC.recvFromArduino(0.1)
		dataInput = np.fromstring(dataInput, dtype=int, sep=',')
		print(dataInput)
		if dataInput == "<<" or dataInput == ">>":
			dataInput = "nothing"
			print ("DataInput %s" %(dataInput))
		if (dataInput.any()):
			processData(dataInput)
			update_row(table,dataInput[0], dataInput)		#Update the table.  col 0 is the row number 
			print(dataInput[0])
		time.sleep(checkDelay)

		if OvenDataObject[2].Status == 4:
			SaveData(2)
			#OvenDataObject.Status = 5						#Temperature Data file saved

	print("Finished Listening")
	print(OvenDataObject[2].Temps)


#Saves a 2D array to a text file from Global OvenDataObject
def SaveData(OvenID):

	global dir_name, error_msg

	#File Saving Stuff
	#===========================
	file_name = "Test_Data"
	dir_name = "C:\JCS\OvenMonitorCtrl"
	#dir_filename = dir_name + '/' + file_name + '{:03d}'.format(sample_count) + '.csv'
	dir_filename = dir_name + '/' + file_name + '.csv'
	if debug:print(dir_filename)
	
	try:        #Prevent the user from acidently overwriting old data.
		df = read_csv(dir_filename)
		error_msg = "File already exists, Please use a different file name"
		return
	except:
		0

	if debug:print("dir_filename: " + dir_filename)


	with open(dir_filename, 'x') as f:          #Creates a file if none exists.  Returns error if it does. 
		if debug:print('Writing data to ' + dir_filename, end='')
		# Write a header to the file
		for chan_num in range(8):
			f.write('Channel ' + str(chan_num) + ',')
		f.write(u'\n')

		if debug:print(len(OvenDataObject[OvenID].Temps))
		for row in OvenDataObject[OvenID].Temps:
			f.write(",".join(str(item) for item in row) + "\n")


	#===========================

class DataClass():
	def __init__(self):
		self.Temps = []
		self.OvenID: int
		self.Count: int
		self.Status = 0
	def append_array(self, Temps):
		self.Temps.append(Temps)

#An array of class objects.  First element is empty so indexing can be done with OvenID number (which starts at 1)
OvenDataObject = [0, DataClass(),DataClass(),DataClass(),DataClass(),DataClass(),DataClass(),DataClass(),DataClass()]
	

#======================
#OvenDataObject = []
# function to illustrate the concept of dealing with the data
def processData(dataRecvd):
	global displayVal
	inputData = dataRecvd
	displayVal.set(dataRecvd)
	global OvenDataObject
	OvenID = dataRecvd[0]
	OvenDataObject[OvenID].OvenID = OvenID
	OvenDataObject[OvenID].Status = dataRecvd[10]
	OvenDataObject[OvenID].append_array([dataRecvd[2], dataRecvd[3], dataRecvd[4], dataRecvd[5],dataRecvd[6], dataRecvd[7], dataRecvd[8], dataRecvd[9]])
	

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
	table.item(table.get_children()[row_index-1], values=list(new_values))



#============================
# Create a treeview table
def build_table(masterframe):

	global table		
	table = ttk.Treeview(
		masterframe, 
		columns= ('ovenid','count', 'temp1','temp2','temp3','temp4','temp5','temp6','temp7','temp8'),
		show='headings')

	table.grid(row=6,column=1,columnspan = 2, ipadx=10, ipady=50)	#Required to show table in masterframe
	
	#build the table columns
	table.column('ovenid', width=50)
	table.heading('ovenid', text='Transmitter')
	table.column('count', width=50)
	table.heading('count', text='Sample #')
	for num in range(1,9):
		table.column('temp{}'.format(num),width=50)
		table.heading('temp{}'.format(num), text='Temp {}'.format(num))

	# Insert some data into the table
	data = [(' ', ' '), (' ', ' '), (' ', ' '),(' ', ' '),(' ', ' '),(' ', ' '),(' ', ' ')]
	for item in data:
		table.insert('', 'end', values=item)