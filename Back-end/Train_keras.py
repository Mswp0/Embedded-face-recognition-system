import dlib
import keras.src.callbacks
from sklearn.utils import shuffle
from keras import utils
import tensorflow as tf
from keras.models import Sequential
from tensorflow.keras.layers import Dense, Activation
from tensorflow.keras.optimizers import Adam, SGD
from keras.metrics import categorical_crossentropy
import cv2
import numpy as np
import sys
import subprocess
from Utilidades import mydb, face_encodings, get_image_paths

def load_config(filename):
    """Carga la configuración desde un archivo de texto."""
    config = {}
    with open(filename, 'r') as file:
        for line in file:
            if ':' in line:
                key, value = line.strip().split(':', 1)
                config[key.strip()] = value.strip()
    return config

# Cargar configuración
config = load_config('myfile.txt')

# Obtener la IP y otros parámetros desde la configuración
db_ip = config.get('DB_IP', 'default_ip')  # Reemplaza 'default_ip' con una IP por defecto si es necesario

# Crear la conexión a la base de datos usando la IP del archivo de configuración
my_DB = mydb(db_ip, "Local_Test", "L0c41_T3st_DB", "usuarios")

# Carga el face encoder
face_encoder = dlib.face_recognition_model_v1("/home/usuario/Python_face detection/dlib_face_recognition_resnet_model_v1.dat")
VALID_EXTENSIONS = ['.jpeg']

# Extrae de la DB los datos para saber si se puede entrenar o no y con qué clases
val = my_DB.read_DB('users', ['id', 'n_im', 'folder'], ['INT', 'INT', 'TEXT'])
print(val)
val_new = [[], [], []]
for x, y in zip(val[0], val[1]):
    val_new[0].append(val[0][x])
    if y > 40:
        val_new[1].append(val[1][x])
        val_new[2].append(val[2][x])
#print(val_new)
# Más de 2 clases entrenables?
if len(val_new[1]) <= 1:
    print('No imágenes suficientes')
    sys.exit()
else:
    print('Creando modelo')

# Almacena las labels y el path de las clases entrenables
class_id = val_new[0]
class_paths = val_new[2]

# Recopilas los paths y el valor de cada cara codificada
list_encodings = []
image_paths, list_id = get_image_paths(class_paths, class_id, VALID_EXTENSIONS)

for image_path in image_paths:
    # Load the image
    image = cv2.imread(image_path)

    # Get the face embeddings
    if image is not None:
        encodings = face_encodings(image, face_encoder)[0]
        list_encodings.append(encodings)

list_encodings = np.array(list_encodings)
list_id = np.array(list_id)

# Prepara los valores con los que se entrenará el clasificador
labels = []
id_weight = []

## Calcula el peso de cada clase para corregir dataset desbalanceado
n_ids, n_img_ids = np.unique(list_id, return_counts=True)
for items in n_ids:
    id_weight.append(len(list_id) / n_img_ids[items])

weight_dict = dict(zip(n_ids, id_weight))
print(weight_dict)

## Prepara las labels en formato one-hot para entrenar con cross-entropy, ej [0 0 1 0] = id 2
for items in list_id:
    this_label = np.zeros(len(class_id))
    this_label[items] = 1
    labels.append(this_label)
labels = np.array(labels)

## Randomiza el orden de los valores codificados e ids
train_labels, train_samples = shuffle(labels, list_encodings)

inputs = keras.layers.Input(shape=(128,))
## Estructura del modelo
x = Dense(10, input_shape=(128,), activation='relu')(inputs)
predictions = Dense(4, activation='softmax')(x)
model = keras.Model(inputs=inputs, outputs=predictions)

# model.summary()

# Prepara parámetro de entrenamiento
model.compile(optimizer=SGD(learning_rate=0.01), loss='categorical_crossentropy', metrics=['categorical_accuracy'])
earlyStopping = keras.src.callbacks.EarlyStopping(monitor='val_loss', patience=10, verbose=0, mode='min')
mcp_save = keras.src.callbacks.ModelCheckpoint('/home/usuario/Project/Model/mdl_wts.keras', save_best_only=True, monitor='val_loss', mode='min')
# mod_save= keras.src.Model.save('keras_model.keras')
reduce_lr_loss = keras.src.callbacks.ReduceLROnPlateau(monitor='val_loss', factor=0.1, patience=7, verbose=1, epsilon=1e-4, mode='min')

# Lanza el entrenamiento
model.fit(x=train_samples, y=train_labels, validation_split=0.1, batch_size=4, epochs=100, shuffle=True, verbose=2, callbacks=[earlyStopping, mcp_save, reduce_lr_loss], class_weight=weight_dict)
# model.fit(x=train_samples, y=train_labels, validation_split=0.1, batch_size=4, epochs=100, shuffle=True, verbose=2)
model.save('/home/usuario/Project/Model/model_tr.keras')

# face_1 = cv2.imread("/home/usuario/Python_face detection/dataset/train/marc/a65.jpeg")
# print(model.predict(np.array(face_encodings(face_1, face_encoder)).reshape(1, -1)))
# face_1 = cv2.imread("/home/usuario/Python_face detection/dataset/train/carla/a5.jpeg")
# print(model.predict(np.array(face_encodings(face_1, face_encoder)).reshape(1, -1)))

# Running other file using run()
subprocess.run(["python3", "/home/usuario/Descargas/frugally-deep-tensorflow-2-15-0/keras_export/convert_model.py", "/home/usuario/Project/Model/model_tr.keras", "/home/usuario/Project/Model/fdeep_model.json"])
json_send = subprocess.Popen(['python', '/home/usuario/PycharmProjects/Demo/Send_model_json.py'])
json_send.wait()
