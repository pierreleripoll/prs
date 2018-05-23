import os
import time
import sys


ip_address= sys.argv[1]
server_port= sys.argv[2]
file_name1= "bluebonnets-at-twilight-near-san-antonio-1920.jpg"
file_name2= "bluebonnets-at-twilight-near-san-antonio-19201.jpg"
file_name3= "bluebonnets-at-twilight-near-san-antonio-19202.jpg"
file_name4= "bluebonnets-at-twilight-near-san-antonio-19203.jpg"
output_file= "testEmpiriqueMulti.csv"

#name= server_name.split("-")
#suffix= name[0][-2:]
suffix="1"

client_name= "client"+suffix
copy_file_name1= "./copy_" +file_name1
copy_file_name2= "./copy_" +file_name2
copy_file_name3= "./copy_" +file_name3
copy_file_name4= "./copy_" +file_name4
copy_touch_command1= "touch "+ copy_file_name1
copy_touch_command2= "touch "+ copy_file_name2
copy_touch_command3= "touch "+ copy_file_name3
copy_touch_command4= "touch "+ copy_file_name4
os.system(copy_touch_command1)
os.system(copy_touch_command2)
os.system(copy_touch_command3)
os.system(copy_touch_command4)

output= open(output_file, "a")
rtt = 50
size = 50

for size in range(10,30,5):
	for rtt in range(0,100,10):
		server_launch_command= "./server"  +" "+str(size)+" "+str(size)+" "+str(rtt)+" &"
		client_launch_command1= "./" +client_name +" " +ip_address +" " +server_port +" " +file_name1 + " 0" +" &"
		client_launch_command2= "./" +client_name +" " +ip_address +" " +server_port +" " +file_name2 + " 0" +" &"
		client_launch_command3= "./" +client_name +" " +ip_address +" " +server_port +" " +file_name3 + " 0" +" &"
		client_launch_command4= "./" +client_name +" " +ip_address +" " +server_port +" " +file_name4 + " 0"



		os.system(server_launch_command)
		time.sleep(0.5)
		print("launching "+ str(client_name))
		start_time= time.time()
		os.system(client_launch_command1)
		time.sleep(0.5)
		print("launching "+ str(client_name))
		os.system(client_launch_command2)
		time.sleep(0.5)
		print("launching "+ str(client_name))
		os.system(client_launch_command3)
		time.sleep(0.5)
		print("launching "+ str(client_name))
		os.system(client_launch_command4)
		end_time= time.time()
		delta= end_time- start_time -3


		statinfo1= os.stat(copy_file_name1)
		statinfo2= os.stat(copy_file_name2)
		statinfo3= os.stat(copy_file_name3)
		statinfo4= os.stat(copy_file_name4)

		stat_sum= statinfo1.st_size+ statinfo2.st_size+ statinfo3.st_size+ statinfo4.st_size
		speed = int(stat_sum/delta)
		output.write(str(size)+","+str(rtt)+","+ str(speed)+"\n")
		kill_command= "killall server"
		os.system(kill_command)
		kill_command= "killall client1"
		os.system(kill_command)

output.close()

remove_command1= "rm "+ copy_file_name1
remove_command2= "rm "+ copy_file_name2
remove_command3= "rm "+ copy_file_name3
remove_command4= "rm "+ copy_file_name4
os.system(remove_command1)
os.system(remove_command2)
os.system(remove_command3)
os.system(remove_command4)



kill_command= "killall server"
os.system(kill_command)
kill_command= "killall client1"
os.system(kill_command)
