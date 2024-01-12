



#======================

# global variables for module

dir_name = "C:\JCS\OvenMonitorTime"   #Save location for .csv data.

inputData = ""
threadRun = True
checkDelay = 0.2 # seconds

testVar = "testvar"

displayVal = StringVar() # for use in the mainscreen
displayVal.set("starting")

global debug
#debug = 0   #Debug mode off.
debug = 1   #Print Statments
#debug = 2  #skips datacollection

#======================