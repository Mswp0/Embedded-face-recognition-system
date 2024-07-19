import socket
import subprocess
import sys
import pickle

file = open('myfile.txt', 'r')
Lines = file.readlines()

count = 0
for line in Lines:

    count += 1
    try:
        line_split = line.split(':')
    except:
        print('Linea no valida')
        continue

    if (line_split[0] == 'MyIP'):
        SERVER_HOST = line_split[1]
    elif (line_split[0] == 'MyLaunch_Port'):
        SERVER_PORT = int(line_split[1])
    else:
        continue
file.close()

try:
    len(SERVER_HOST)
    len(str(SERVER_HOST))
except:
    print("Error Sock_Train: Faltan datos en el txt")
    exit()

BUFF_SIZE = 4096
HEADERSIZE = 5
train_proc = None

s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
try:
    s.bind((SERVER_HOST, SERVER_PORT))
except:
    print("Error Sock_Train: Fallo al establecer el socket")
    exit()

s.listen(5)
print(f"[*] Listening as {SERVER_HOST}:{SERVER_PORT}")

while True:
    client_socket, address = s.accept()
    print(f"[+] {address} is connected.")
    full_msg = ''
    new_msg = True
    while True:
        msg = client_socket.recv(BUFF_SIZE)
        if new_msg:
            print(f"New message length: {msg[:HEADERSIZE]}")
            try:
                msglen = int(msg[:HEADERSIZE])
            except:
                continue
            new_msg = False
        full_msg += msg.decode("utf-8")

        if len(full_msg) - HEADERSIZE == msglen:
            print(full_msg)
            print(full_msg[HEADERSIZE:])
            if full_msg[HEADERSIZE:] == "Launch Training":
                if train_proc is None or train_proc.poll() is not None:
                    train_proc = subprocess.Popen(['python', '/home/usuario/PycharmProjects/Demo/train_keras.py'])
                    train_proc.wait()
                    #json_send = subprocess.Popen(['python', '/home/usuario/PycharmProjects/Demo/Send_model_json.py'])
            new_msg = True
            full_msg = ''
            break
    client_socket.close()
