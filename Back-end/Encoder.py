import socket
import numpy as np
import struct
import dlib
import cv2

# Cargar el modelo de reconocimiento facial
face_encoder = dlib.face_recognition_model_v1("/home/usuario/Python_face_detection/dlib_face_recognition_resnet_model_v1.dat")

def face_encodings(image):
    image = cv2.resize(image, (150, 150), interpolation=cv2.INTER_LINEAR)
    return [np.array(face_encoder.compute_face_descriptor(image))]

def process_image(image_array):
    # Esta función debe realizar algún tipo de procesamiento en la imagen
    # y retornar un vector de 128 valores decimales. Aquí, simplemente se
    # genera un vector de ejemplo.
    emb = face_encodings(image_array)[0]
    print(emb)
    return emb

def main():
    # Leer la IP y el puerto del servidor desde myfile.txt
    file = open('myfile.txt', 'r')
    Lines = file.readlines()
    file.close()

    for line in Lines:
        try:
            line_split = line.split(':')
            if line_split[0] == 'MyIP':
                SERVER_IP = line_split[1].strip()
            elif line_split[0] == 'MyServer_Port':
                SERVER_PORT = int(line_split[1].strip())
        except:
            print('Error al leer la línea del archivo de configuración.')
            continue

    # Crear el socket del servidor
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((SERVER_IP, SERVER_PORT))
    server_socket.listen(1)
    print(f"Servidor escuchando en {SERVER_IP}:{SERVER_PORT}")

    while True:
        client_socket, addr = server_socket.accept()
        print("Conexión aceptada de", addr)

        # Recibir el tamaño del vector de la imagen
        img_size_data = client_socket.recv(4)
        img_size = struct.unpack('!I', img_size_data)[0]

        # Recibir la imagen
        img_data = b''
        while len(img_data) < img_size:
            packet = client_socket.recv(img_size - len(img_data))
            if not packet:
                break
            img_data += packet

        # Convertir los datos recibidos en una matriz de numpy
        image_array = np.frombuffer(img_data, dtype=np.uint8).reshape((150, 150, 3))

        # Procesar la imagen
        result_vector = process_image(image_array)
        print(result_vector)
        # Enviar el vector resultante
        client_socket.sendall(result_vector.tobytes())
        print("Vector enviado")
        # Cerrar la conexión
        client_socket.close()

if __name__ == "__main__":
    main()
