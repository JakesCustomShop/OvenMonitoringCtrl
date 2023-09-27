from audioop import findfit
import tkinter as tk
from tkinter import *
from tkinter import ttk
from turtle import width
import arduinoCommsB as aC
import atexit
import numpy as np



#=========================
#  code to ensure a cleaan exit

def exit_handler():
    print ('My application is ending!')
    cD.stopListening()
    aC.closeSerial()

atexit.register(exit_handler)

#===========================
# global variables for this module

ledAstatus = 0
ledBstatus = 0
servoPos = 10

tkArd = tk.Tk()
tkArd.minsize(width=320, height=170)
tkArd.config(bg = 'yellow')
tkArd.title("Arduino GUI Demo")

	# the next line must come after  tkArd = Tk() so that a StringVar()
	#   can be created in checkForData.
import arduinoCheckForData as cD # so we can refer to its variables

#======================
# get first view ready

def setupView():
	global masterframe
	masterframe = tk.Frame(bg = "yellow")
	masterframe.pack()
	
	selectPort()
		
#======================
# definition of screen to choose the Serial Port

def selectPort():
	global masterframe, radioVar
	for child in masterframe.winfo_children():
		child.destroy()
	radioVar = tk.StringVar()

	lst = aC.listSerialPorts()
	
	l1= tk.Label(masterframe, width = 5, height = 2, bg = "yellow") 
	l1.pack()

	if len(lst) > 0:
		for n in lst:
			r1 = tk.Radiobutton(masterframe, text=n, variable=radioVar, value=n, bg = "yellow")
			r1.config(command = radioBtnPress)
			r1.pack(anchor=W) # python 2.x use WEST
	else:
		l2 = tk.Label(masterframe, text = "No Serial Port Found")
		l2.pack()



#def update_row(table, row_index, new_values):
#	print("Updating Table")
#	table.item(table.get_children()[row_index], values=new_values)
#======================
# definition of main screen to control Arduino
	
def mainScreen():
	global masterframe
	for child in masterframe.winfo_children():
		child.destroy()
		
	labelA = tk.Label(masterframe, width = 5, height = 2, bg = "yellow") 
	labelB = tk.Label(masterframe, width = 5, bg = "yellow") 
	labelC = tk.Label(masterframe, width = 5, bg = "yellow") 
	
	ledAbutton = tk.Button(masterframe, text="LedA", fg="white", bg="black")
	ledAbutton.config(command = lambda: btnA(ledAbutton))
	
	ledBbutton = tk.Button(masterframe, text="LedB", fg="white", bg="black")
	ledBbutton.config(command = lambda:  btnB(ledBbutton))
	
	slider = tk.Scale(masterframe, from_=10, to=170, orient=HORIZONTAL) # python 2.x use HORIZONTAL
	slider.config(command = slide)
	
	labelD = tk.Label(masterframe, width = 5, bg = "yellow") 
	labelE= tk.Label(masterframe, textvariable = cD.displayVal) 
	
	labelA.grid(row = 0)
	ledAbutton.grid(row = 1)
	labelB.grid(row = 1, column = 2)
	ledBbutton.grid(row = 1, column = 3)
	labelC.grid(row = 2)
	slider.grid(row = 3, columnspan = 4)
	labelD.grid(row = 4)
	labelE.grid(row = 5, columnspan = 4)


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




