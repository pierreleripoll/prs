import os
import time
import sys

server_name= sys.argv[1]
ip_address= sys.argv[2]
server_port= sys.argv[3]
file_name= sys.argv[4]
output_file= "testEmpirique.txt"

#name= server_name.split("-")
#suffix= name[0][-2:]
suffix="1"

client_name= "client"+suffix

output= open(output_file, "a")

for rtt in range(1,50,1):
    server_launch_command= "./" +server_name +" " +server_port +" "+" 18 "+" "+" 18 "+" "+str(rtt)+" &"
    client_launch_command= "./" +client_name +" " +ip_address +" " +server_port +" " +file_name + " 0"
    speed = 0
    os.system(server_launch_command)
    for i in range(20):
        start_time= time.time()
        os.system(client_launch_command)
        end_time= time.time()
        delta= end_time- start_time
        print("programme fini")

        copy_file_name= "./copy_" +file_name
        statinfo= os.stat(copy_file_name)
        speed += int(statinfo.st_size/delta)

    speed = speed/20
    output.write(str(rtt)+";"+ str(speed)+"\n")

    kill_command= "killall server"
    os.system(kill_command)
    kill_command= "killall client*"
    os.system(kill_command)

output.close()

remove_command= "rm "+ copy_file_name
os.system(remove_command)

kill_command= "killall server"
os.system(kill_command)
kill_command= "killall client*"
os.system(kill_command)
