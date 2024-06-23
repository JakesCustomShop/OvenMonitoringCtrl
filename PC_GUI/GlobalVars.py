#======================
# global variables used throughout GUI python files.

#Global value for file save location

global dir_name
dir_name = "C:/JCS/OvenMonitorTime"   #Save location for .csv data.
global save_config
global col_header
col_header  = ["col 1","col 2","col 3","col 4","col 5","col 6","col 7","col 8"]	#colum header names. 
global num_rows     #number of rows in the Treeview Table.
num_rows = 20
global row_header
row_header = [" "]*num_rows
global user_comment
user_comment = "User Comment"

#debug = 0   #Debug mode off.
debug = 1   #Print Statments
#debug = 2  #skips datacollection.  Create a datafile with header only

#======================