from tkinter import *
import threading
import time

import arduinoCommsB as aC


#======================

# global variables for module

inputData = ""
threadRun = True
checkDelay = 2 # seconds

testVar = "testvar"

displayVal = StringVar() # for use in the mainscreen
displayVal.set("starting")

#======================

def checkForData():
	global threadRun, checkDelay
	print ("Starting to Listen")
	while threadRun == True:
		dataInput = aC.recvFromArduino(0.1)
		#~ print ("DataInput %s" %(dataInput))
		if dataInput == "<<" or dataInput == ">>":
			dataInput = "nothing"
		processData(dataInput)
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