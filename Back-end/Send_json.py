import socket
import os
import json

# Función para cargar configuración desde un archivo
def load_config(filename):
    config = {'ClientIP': []}
    with open(filename, 'r') as file:
        lines = file.readlines()
        for line in lines:
            try:
                key, value = line.strip().split(':', 1)
                if key in config:
                    config[key].append(value.strip())
            except ValueError:
                print(f'Linea no válida: {line.strip()}')
    return config

# Configuración
config = load_config('myfile.txt')
ClientIP = config['ClientIP']

SEPARATOR = "<>"
BUFF_SIZE = 200
PORT = 6001  # Puerto fijo

filename = "fdeep_model.json"
filepath = "/home/usuario/Project/Model/"
file_path = os.path.join(filepath, filename)
resto = "None"
filesize = os.path.getsize(file_path)

# Protocolo de envío de datos
def send_file(host):
    with socket.socket() as s:
        try:
            print(f"Connecting to {host}:{PORT}")
            s.connect((host, PORT))
            print(f"Sending file info: {filename}, Size: {filesize}")
            s.sendall(f"INFO{SEPARATOR}FILENAME<>{filename}{SEPARATOR}FILESIZE<>{filesize}{SEPARATOR}{resto}".encode('utf-8'))

            with open(file_path, "r") as f:
                json_data = json.load(f)
            json_str = json.dumps(json_data)
            s.sendall(json_str.encode('utf-8'))
            print(f"File {filename} sent successfully.")
        except Exception as e:
            print(f"Error occurred: {e}")

# Enviar archivo a todos los clientes
for host in ClientIP:
    send_file(host)
