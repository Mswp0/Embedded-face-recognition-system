import sqlite3
import _mysql_connector
import mysql.connector
from glob import glob
import cv2
import numpy as np
import os
import torch
from torch.utils.data import Dataset, DataLoader
import torch.nn as nn
import logging
import time
import random

def rotation(img, angle):
    angle = int(random.uniform(-angle, angle))
    h, w = img.shape[:2]
    M = cv2.getRotationMatrix2D((int(w/2), int(h/2)), angle, 1)
    img = cv2.warpAffine(img, M, (w, h))
    return img
def horizontal_flip(img):
    if random.randint(0, 1):
        return cv2.flip(img, 1)
    else:
        return img

def save_im(img,file_name,new_path,i,db):
    if random.randint(0,100)<75:
        cv2.imwrite(new_path+file_name,img)
        print(new_path+file_name)
        n_im = db.read_DB('users', ['n_im'], ['INT'])
        db.write_DB('users', ['n_im'], [n_im[0][i] + 1], i)
    else:
        path=new_path.replace('/train/', '/val/')
        cv2.imwrite(path + file_name, img)
    return

class mydb:
    def __init__(self,host,user,password,db_used):
        self.host=host
        self.user=user
        self.password=password
        self.db_use=db_used

    def write_DB(self,tabla, parametros, valores, filtro):
        conn = mysql.connector.connect(
            host=self.host,user=self.user,password=self.password,database=self.db_use)

        c = conn.cursor()
        try:
            if tabla == 'servicios':
                for i, j in zip(parametros, valores):
                    c.execute("UPDATE {T} SET {P}={V} ".format(T=tabla, P=i, V=j))
            elif tabla == 'users':
                for i, j in zip(parametros, valores):
                    c.execute("UPDATE {T} SET {P}='{V}' WHERE id={F}".format(T=tabla, P=i, V=j, F=filtro))
        except:
            print('Error 02: Fallo al actualizar BD')
        conn.commit()
        conn.close()


    def read_DB(self,tabla, parametros, tipos):
        read = []
        conn = mysql.connector.connect(
            host=self.host,user=self.user,password=self.password,database=self.db_use)
        c = conn.cursor()

        try:
            for i, j in zip(parametros, tipos):
                c.execute("SELECT {P} FROM {T}".format(P=i, T=tabla))
                value = (c.fetchall())
                if j == 'INT':
                    read.append([row[0] for row in value])
                elif j == 'TEXT':
                    read.append([''.join(row) for row in value])
        except:
            print('Error 03: Fallo al leer BD')
            read = [[-1,-1],[-1,-1]]
        conn.commit()
        conn.close()
        return read


def permiso(i, Mx,db):
    ##Comprobar si tiene permisos
    val = db.read_DB('users', ['permission'], ['TEXT'])
    perm = val[0][i]
    val = db.read_DB('users', ['name'], ['TEXT'])
    name = val[0][i]
    if (perm[int(Mx[1])]):
        print("Servicio {M} habilitado para {N}".format(M=Mx, N=name))
        db.write_DB('servicios', [Mx], ['1'], 0)
        return 1
    else:
        print("Acceso denegado {N} no tiene permisos para {M}".format(M=Mx, N=name))
        return 0

def face_encodings(image,face_encoder):
    image=cv2.resize(image, (150,150), interpolation = cv2.INTER_LINEAR)
    #image=cv2.cvtColor(image,cv2.COLOR_GRAY2RGB)
    return [np.array(face_encoder.compute_face_descriptor(image))]


def get_image_paths(image_folders, ids, VALID_EXTENSIONS):
    image_paths = []
    image_ids = []
    for folder, n_id in zip(image_folders, ids):
        # print(folder)
        class_file_paths = glob(os.path.sep.join([folder, '*.*']))
        # print(class_file_paths)
        for file_path in class_file_paths:
            ext = os.path.splitext(file_path)[1]

            if ext.lower() not in VALID_EXTENSIONS:
                print("Skipping file: {}".format(file_path))
                continue
            image_ids.append(n_id)
            image_paths.append(file_path)
    return image_paths, image_ids