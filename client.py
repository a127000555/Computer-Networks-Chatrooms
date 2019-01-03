import json
import sys
import time
from pwn import p64,u64
from termcolor import *
from socket import *
from base64 import *
from pathlib import Path
user_list = []
def new_connection(serverName='localhost',serverPort=int(sys.argv[1])):
	clientSocket = socket(AF_INET, SOCK_STREAM)
	clientSocket.connect((serverName,serverPort))
	return clientSocket

def protocol_pkt(operation, content):
	content_json = json.dumps(content).encode()
	uni_pkt = operation[0].lower().encode() + p64(len(content_json))
	return uni_pkt, content_json

def wait_response(clientSocket):
	s = clientSocket.recv(9)
	l = u64(s[1:])
	s = clientSocket.recv(l)
	print('recv:' , s)
	j = json.loads(s.decode())
	return j

def sign_up(clientSocket, usr, pwd):
	uni_pkt, content = protocol_pkt('s', {'username':usr,"password":pwd})
	clientSocket.sendall(uni_pkt)
	clientSocket.sendall(content)
	j = wait_response(clientSocket)
	print(colored(j['state'],'cyan'))
def show_user_list(user_list):
	print('='*25)
	for p in user_list:
		print('%s %5d' % (p[1].ljust(15),p[0]))
	print('='*25)
def login(clientSocket, usr , pwd):
	global user_list
	uni_pkt, content = protocol_pkt('l', {'username':usr,"password":pwd})
	clientSocket.sendall(uni_pkt)
	clientSocket.sendall(content)
	j = wait_response(clientSocket)
def messaging(clientSocket, i, msg):
	msg = b64encode(msg.encode()).decode()
	uni_pkt, content = protocol_pkt('m', {'target':i,"type":"message","message":msg})
	clientSocket.sendall(uni_pkt)
	clientSocket.sendall(content)
	j = wait_response(clientSocket)
	print(colored(j['state'],'cyan'))

def upload_file(clientSocket, target, file_path):
	mf = Path(file_path)
	if not mf.exists():
		print('file not exists.')
	else:
		fin = open(file_path,'rb')
		msg = b64encode(fin.read()).decode()
		filename = file_path.split('/')[-1]
		uni_pkt, content = protocol_pkt('u', {'target':target,"filename":filename,"data":msg})
		clientSocket.sendall(uni_pkt)
		clientSocket.sendall(content)
		j = wait_response(clientSocket)
		print(colored(j['state'],'cyan'))

def download(clientSocket, target, filename, file_path):
	uni_pkt, content = protocol_pkt('d', {'target':target,"filename":filename})
	clientSocket.sendall(uni_pkt)
	clientSocket.sendall(content)
	j = wait_response(clientSocket)
	print(j['data'])
	data_bytes = b64decode(j['data'].encode())
	with open(file_path,'wb') as fout:
		fout.write(data_bytes)
	print(colored(j['state'],'cyan'))

def get_user_list(clientSocket, target):
	if target not in ['b','f','a']:
		print('The target is not recogniazable.')
		return
	uni_pkt, content = protocol_pkt('a', {'target':target})
	clientSocket.sendall(uni_pkt)
	clientSocket.sendall(content)
	j = wait_response(clientSocket)
	if j['status_code'] == 200:
		show_user_list(j['data'])

def refresh(clientSocket, i, start,end):
	uni_pkt, content = protocol_pkt('r', {'target':i,"start_from":start,"end_to":end})
	clientSocket.sendall(uni_pkt)
	clientSocket.sendall(content)
	j = wait_response(clientSocket)
	print(colored(j['state'],'cyan'))
	if j['status_code'] == 200:
		for k,d in sorted(j["data"]):
			if d["type"] == "message":
				print("line {:0>4d} :[{: <40s}] (talk:{})".format(k,b64decode(d["message"]).decode(),d["who"]))
			else:
				print(colored('line %04d :a file uploaded by %d ! (%s)' % (k,d['who'],d['message']),'red'))
def edit_list(clientSocket, op,target_list, target_id):
	if op not in ["add",'delete'] or target_list not in ['black','friend']:
		print('what?')
		return
	uni_pkt, content = protocol_pkt('x', {'op':op ,'target_id':target_id,'target_list':target_list})
	clientSocket.sendall(uni_pkt)
	clientSocket.sendall(content)
	j = wait_response(clientSocket)
	print(colored(j['state'],'cyan'))
def crash_packet(clientSocket):
	uni_pkt, content = protocol_pkt('x', {'opzzz':1 ,'targzet_id':'z','tazzzzrget_list':{}})
	clientSocket.sendall(uni_pkt)
	clientSocket.sendall(content)
	j = wait_response(clientSocket)
	print(colored(j['state'],'cyan'))

def id_2_name(clientSocket,target):
	uni_pkt, content = protocol_pkt('t', {'target':target})
	clientSocket.sendall(uni_pkt)
	clientSocket.sendall(content)
	j = wait_response(clientSocket)
	print(colored(j['state'],'cyan'))

clientSocket = new_connection()
while True:
	command = input('command? (sign up / login / show user / messaging / refresh / edit list / upload file)\n[>]: ').strip().lower()
	if command == 'sign up':
		username = input('username?\n[>]: ').strip()
		password = input('password?\n[>]: ').strip()
		sign_up(clientSocket,username,password)
	elif command == 'login':
		username = input('username?\n[>]: ').strip()
		password = input('password?\n[>]: ').strip()
		login(clientSocket,username,password)
	elif command == 'show user':
		target = input('target? (b/f/a)\n[>]: ').strip()
		get_user_list(clientSocket,target)
	elif command == 'upload file':
		target = int(input('target_id?\n[>]: ').strip())
		file_path = input('file path? \n[>]: ').strip()
		upload_file(clientSocket,target,file_path)
	elif command == 'download file':
		target = int(input('target_id?\n[>]: ').strip())
		filename = input('filename? \n[>]: ').strip()
		file_path = input('save where? \n[>]: ').strip()
		download(clientSocket,target,filename,file_path)
	elif command == 'messaging':
		target = int(input('to which id?\n[>]: ').strip())
		msg = input('Say something\n[>]: '.strip())
		messaging(clientSocket,target,msg)
	elif command == 'refresh all':
		target = int(input('to which id?\n[>]: ').strip())
		refresh(clientSocket,target,0,-1)
	elif command == 'refresh':
		target = int(input('to which id?\n[>]: ').strip())
		start_from = int(input('start from? \n[>]: ').strip())
		end_to = int(input('end to? \n[>]: ').strip())
		refresh(clientSocket,target,start_from,end_to)
	elif command == 'edit list':
		target_list = input('to what list? (black/friend) \n[>]: ').strip()
		op = input('add or delete\n[>]: ').strip()
		target_id = int(input('to which id?\n[>]: ').strip())
		edit_list(clientSocket,op,target_list,target_id)
	elif command == 'auto login':
		login(clientSocket,'mama','mama')
	elif command == 'auto edit list':
		edit_list(clientSocket,'add','black',15)
	elif command == 'auto send trash':
		target = int(input('to which id?\n[>]: ').strip())
		for i in range(100):
			messaging(clientSocket,target,str(time.localtime())+str(i))
	elif command == 'auto send file':
		upload_file(clientSocket,2,'./sample_file')
	elif command == 'go crash':
		crash_packet(clientSocket)
	elif command == 'id 2 name':
		target = int(input('to which id?\n[>]: ').strip())
		id_2_name(clientSocket,target)
clientSocket.close()

'''
login
duck
duck
messaging
1
OAO
'''