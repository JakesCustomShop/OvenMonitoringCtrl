from msilib import Table
from tkinter import *
from tkinter import ttk
import threading
import time
import numpy as np
import arduinoCommsB as aC

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
	global threadRun, checkDelay
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
	print ("Finished Listening")

#======================

# function to illustrate the concept of dealing with the data
def processData(dataRecvd):
	global displayVal
	inputData = dataRecvd
	displayVal.set(dataRecvd)

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