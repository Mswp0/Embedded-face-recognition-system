# Importar la biblioteca de sockets
import socket
import subprocess
import threading
import os
import sys
from time import sleep


def sck_img(s):
    BUFF_SIZE = 4096
    SEPARATOR = "<>"
    path_img = "/home/usuario/Project/Faces/"
    s.listen(5)
    while True:
        print(f"[*] Listening as ")
        client_socket, address = s.accept()
        print(f"[+] {address} is connected.")
        try:
            recived = client_socket.recv(BUFF_SIZE).decode(encoding="utf-8")
            print(recived)
            MSG_flg, id, folder, filename, filesize, resto = recived.split(SEPARATOR, -1)
            print(MSG_flg)
            if MSG_flg == 'INFO':
                print("Eviando confirmacion")
                client_socket.send(("INFO RECV").encode(encoding="utf-8"))
                MSG_flg = 'IMG'
            if MSG_flg == 'IMG':
                filename = os.path.basename(filename)
                filesize = int(filesize)
                if folder == 'Train':
                    with open(path_img + "Train/Id_" + id + "/" + filename, "wb") as f:
                        while True:
                            bytes_read = client_socket.recv(BUFF_SIZE)
                            print(bytes_read)
                            if not bytes_read:
                                print("Final archivo")
                                client_socket.send(("IMG RECV").encode(encodings="utf-8"))
                                break
                            f.write(bytes_read)
                else:
                    with open(path_img + "Val/Id_" +id + "/" + filename, "wb") as f:
                        while True:
                            bytes_read = client_socket.recv(BUFF_SIZE)
                            print(bytes_read)
                            if not bytes_read:
                                print("Final archivo")
                                client_socket.send(("IMG RECV").encode(encodings="utf-8"))
                                break
                            f.write(bytes_read)

            client_socket.close()
        except:
            client_socket.send(("ERROR").encode(encoding="utf-8"))
            client_socket.close()
    s.close()
    sock_list.remove(sock)

if __name__ == '__main__':
    file = open('myfile.txt', 'r')
    Lines = file.readlines()

    count = 0
    # Strips the newline character
    ClientIP = []
    ClientPort = []
    for line in Lines:
        count += 1
        try:
            line_split = line.split(':')
        except:
            print('Linea no valida')
            continue

        if (line_split[0] == 'MyIP'):
            direccion_ip = line_split[1]
        elif (line_split[0] == 'MyMult_Port'):
            puerto = int(line_split[1])
        elif (line_split[0] == 'MyLaunch_Port'):
            n_port_train = int(line_split[1])
        else:
            continue
    file.close()
    try:
        len(direccion_ip)
        len(str(puerto))
        len(str(n_port_train))
    except:
        print("Error Sock_Mult: Faltan datos en el txt")
        exit()

    # Creamos un objeto socket TCP
    sock_list=[]
    thread_list=[]
    servidor = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Definimos el puerto y la dirección IP en la que el servidor escuchará

    # Enlazamos el objeto socket al puerto y dirección IP
    try:
        servidor.bind((direccion_ip, puerto))
    except:
        print("Error Sock_Mult: Fallo al establecer el socket")
        exit()

    # Ponemos el servidor en modo escucha para aceptar conexiones entrantes
    servidor.listen(1)
    while(1):
        print("Esperando conexiones entrantes...")
        # Aceptamos una conexión entrante
        conexion, direccion = servidor.accept()
        print("Conexión establecida desde", direccion)
        # Recibimos datos del cliente
        datos_recibidos = conexion.recv(4096)
        print(datos_recibidos.decode())

        sock_img = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock_img.bind(('', 0))
        n_port_img=sock_img.getsockname()[1]
        sock_list.append(sock_img)


        respuesta = "<>IMG<>" + str(n_port_img) + "<>TRAIN<>" + str(n_port_train) + "<>NONE"

        prd = threading.Thread(target=sck_img, args=(sock_list[-1],))
        thread_list.append(prd)
        thread_list[-1].start()

        # Enviar los datos serializados a través del socket utilizando sendall
        conexion.send(respuesta.encode('utf-8'))
        conexion.close()
    servidor.close()