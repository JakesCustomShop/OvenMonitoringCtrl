#======================
# global variables used throughout GUI python files.

#Global value for file save location

global dir_name
dir_name = "C:/JCS/OvenMonitorTime"   #Save location for .csv data.
global save_config
global col_header
col_header  = ["col 1","col 2","col 3","col 4","col 5","col 6","col 7","col 8"]	#colum header names. 
global row_header
row_header = ["row 1","row 2","row 3","row 4","row 5","row 6","row 7","row 8","row 9","row 10","row 11","row 12"]	#row header names. 
global user_comment
user_comment = "User Comment"
global num_rows     #number of rows in the Treeview Table.
num_rows = 11

#debug = 0   #Debug mode off.
debug = 1   #Print Statments
#debug = 2  #skips datacollection.  Create a datafile with header only

#======================