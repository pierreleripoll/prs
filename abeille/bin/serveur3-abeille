#!/usr/bin/env python3

import socket
import os
import time
import sys
import socket
from math import ceil
import multiprocessing

UDP_IP = "0.0.0.0"
TAILLE = 1500 - 6
UDP_PORT = int(sys.argv[1])
DATA_UDP_PORT = 3457
TIMEOUT = 0.005
TIMEOUT_ASYNC = 0.0025
FENETRE = 300

indexClient = 0

last_packet = []
last_error = []
timeout_error = []

sockinfo = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
sockinfo.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) 
sockinfo.bind((UDP_IP, UDP_PORT))


def send_file_asynchronev2(sockdata, addr, fichier, max_segment, TAILLE, FENETRE,clientIndex, last_packet, last_error, timeout_error):
	numero = 0
	last_error[clientIndex] = -1
	last_packet[clientIndex] = 0
	timeout_error[clientIndex] = -1

	while ((last_packet[clientIndex] < max_segment)):
		for i in range(0, FENETRE):
			numero = numero + 1
			if send_packet(sockdata, addr, fichier, TAILLE, numero) != 0:
				break

			last_packet_fix = last_packet[clientIndex]
			last_error_fix =  last_error[clientIndex]
			timeout_error_fix = timeout_error[clientIndex]
			#print("Send packet : ", numero, int((last_packet_fix*100)/max_segment) ,"%")
			

		if last_error_fix != -1:
			numero = last_error_fix
			replace_file_cursor(fichier, TAILLE, numero)
			last_error_fix = -1

		if ((numero < last_packet_fix) or (numero >= max_segment)):
			numero = last_packet_fix
			replace_file_cursor(fichier, TAILLE, numero)

		if timeout_error_fix == 1:
			numero = last_packet_fix
			replace_file_cursor(fichier, TAILLE, numero)


	print("Fin de la communication")
	for i in range(0,100):
		sockdata.sendto("FIN\0".encode(), addr)
		print("FIN")

def reception_asynchronev2(UDP_IP, DATA_UDP_PORT, max_segment, TIMEOUT_ASYNC, clientIndex, last_packet, last_error, timeout_error):
	sockrecv = socket.socket(socket.AF_INET,socket.SOCK_DGRAM) 
	sockrecv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	sockrecv.settimeout(TIMEOUT_ASYNC)
	sockrecv.bind((UDP_IP, DATA_UDP_PORT))

	while last_packet[clientIndex] < max_segment:
		try:
			ack, addr = sockrecv.recvfrom(9)
			ack = int(ack.decode()[3:])
			timeout_error[clientIndex] = -1
		except:
			ack = -1
			timeout_error[clientIndex] = 1

		if ack == last_packet[clientIndex]:
			last_error[clientIndex] = ack

		elif ack > last_packet[clientIndex]:
			last_packet[clientIndex] = ack
			if last_packet[clientIndex] > last_error[clientIndex]:
				last_error[clientIndex] = -1


	print("Fin de la communication")
	for i in range(0,100):
		sockrecv.sendto("FIN\0".encode(), addr)
		print("FIN")


def send_packet_specific(sockdata, addr, fichier, TAILLE, numero):
	pos = fichier.tell()
	fichier.seek((numero - 1) * TAILLE)

	numero = str(numero).zfill(6)
	data = numero.encode() + fichier.read(TAILLE)
	sockdata.sendto(data,addr)

	fichier.seek(pos)

def replace_file_cursor(fichier, TAILLE, numero):
	fichier.seek(numero * TAILLE)
	return fichier

def send_packet(sockdata, addr, fichier, TAILLE, numero):
	numerostr = str(numero).zfill(6)
	fread = fichier.read(TAILLE)
	data = numerostr.encode() + fread

	sockdata.sendto(data,addr)

	if (fread == b''):
		return numero
	else :
		return 0

def connect(sockinfo, UDP_IP, UDP_PORT ,DATA_UDP_PORT):

	data = bytearray()

	#RECTEPTION SYN
	while (("SYN" in data.decode()) == False):
		data, addr = sockinfo.recvfrom(4)

	#ENVOIE SYN-ACK

	sockdata = socket.socket(socket.AF_INET,socket.SOCK_DGRAM) 
	sockdata.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	sockdata.bind((UDP_IP, DATA_UDP_PORT))

	print("Port data : "+str(DATA_UDP_PORT).zfill(4))

	sockinfo.sendto(("SYN-ACK"+str(DATA_UDP_PORT)+"\0").encode(),addr)

	while (("ACK" in data.decode()) == False):
		data, addr = sockinfo.recvfrom(4)

	print("Connection Etablie !!")
	return sockdata


def ouvrir_fichier(sockdata):

	data, addr = sockdata.recvfrom(1024)
	data = data.decode()[:-1]
	
	fichier = open(data, "rb")
	print("Ouverture du fichier : " + data)
	taille_fichier = os.path.getsize(data)

	return addr, fichier, taille_fichier

while 1 :
	
	sockdata = connect(sockinfo, UDP_IP, UDP_PORT, DATA_UDP_PORT)

	last_packet = multiprocessing.Array('i', 10)
	last_error = multiprocessing.Array('i',10)
	timeout_error = multiprocessing.Array('i',10)


	addr, fichier, taille_fichier = ouvrir_fichier(sockdata)
	max_segment = ceil(taille_fichier/TAILLE)
	print("Nombre de segment = ",  max_segment)

	p = multiprocessing.Process(target=reception_asynchronev2, args=(UDP_IP, DATA_UDP_PORT, max_segment,TIMEOUT_ASYNC,indexClient,last_packet, last_error, timeout_error, ))
	p.start()

	p = multiprocessing.Process(target=send_file_asynchronev2, args=(sockdata, addr, fichier, max_segment, TAILLE, FENETRE,indexClient,last_packet, last_error, timeout_error,))
	p.start()

	indexClient = indexClient + 1
	DATA_UDP_PORT = DATA_UDP_PORT + 1
