import os
import shutil
import tkinter as tk
from tkinter import filedialog, messagebox, Scrollbar, Listbox
from PIL import Image, ImageTk
from Utilidades import mydb

# Leer la IP y el puerto desde myfile.txt
file = open('myfile.txt', 'r')
Lines = file.readlines()
file.close()

for line in Lines:
    try:
        line_split = line.split(':')
        if (line_split[0] == 'MyIP'):
            SERVER_HOST = line_split[1].strip()
        elif (line_split[0] == 'MyLaunch_Port'):
            SERVER_PORT = int(line_split[1].strip())
    except:
        print('Error al leer la línea del archivo de configuración.')
        continue

# Crear una instancia de mydb usando los valores leídos
my_DB = mydb(SERVER_HOST, "Local_Test", "L0c41_T3st_DB", "usuarios")

class ImageManager:
    def __init__(self, root):
        self.root = root
        self.root.title("Image Manager")

        # Frame para la lista de imágenes
        self.frame = tk.Frame(self.root)
        self.frame.pack(side=tk.LEFT, padx=10, pady=10)

        # Lista de imágenes
        self.image_list = Listbox(self.frame, height=20, width=50)
        self.image_list.pack(side=tk.LEFT, fill=tk.BOTH, expand=1)
        self.image_list.bind("<<ListboxSelect>>", self.show_image)

        # Barra de desplazamiento para la lista
        self.scrollbar = Scrollbar(self.frame)
        self.scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        # Configuración de la barra de desplazamiento y la lista
        self.image_list.config(yscrollcommand=self.scrollbar.set)
        self.scrollbar.config(command=self.image_list.yview)

        # Frame para la vista previa de la imagen
        self.preview_frame = tk.Frame(self.root)
        self.preview_frame.pack(side=tk.RIGHT, padx=10, pady=10)

        self.preview_label = tk.Label(self.preview_frame)
        self.preview_label.pack()

        # Botones
        self.btn_delete = tk.Button(self.root, text="Eliminar Imagen", command=self.delete_image)
        self.btn_delete.pack(pady=10)
        self.btn_move = tk.Button(self.root, text="Mover Imagen", command=self.move_image)
        self.btn_move.pack(pady=5)
        self.btn_change_folder = tk.Button(self.root, text="Cambiar Carpeta", command=self.change_folder)
        self.btn_change_folder.pack(pady=10)
        self.btn_exit = tk.Button(self.root, text="Finalizar Aplicación", command=self.root.quit)
        self.btn_exit.pack(pady=5)

        # Directorio actual
        self.current_directory = ''

        # Cargar imágenes de la carpeta
        self.change_folder()

    def change_folder(self):
        self.current_directory = filedialog.askdirectory(initialdir="/home/usuario/Project/Faces/", title="Selecciona la carpeta de imágenes")

        if self.current_directory:
            self.update_image_list()

    def update_image_list(self):
        self.image_list.delete(0, tk.END)  # Limpiar la lista actual

        # Obtener lista de imágenes en el directorio
        images = [f for f in os.listdir(self.current_directory) if f.endswith(('jpg', 'jpeg', 'png', 'gif'))]

        # Mostrar las imágenes en la lista
        for img in images:
            self.image_list.insert(tk.END, img)

        # Limpiar la vista previa de la imagen
        self.preview_label.config(image='')

    def show_image(self, event):
        # Obtener la imagen seleccionada
        try:
            index = self.image_list.curselection()[0]
            image_name = self.image_list.get(index)
            image_path = os.path.join(self.current_directory, image_name)

            # Cargar y mostrar la imagen
            image = Image.open(image_path)
            image.thumbnail((300, 300))  # Redimensionar la imagen para que quepa en el label
            photo = ImageTk.PhotoImage(image)

            self.preview_label.config(image=photo)
            self.preview_label.image = photo  # Mantener una referencia de la imagen
        except IndexError:
            self.preview_label.config(image='')  # Limpiar la vista previa si no hay selección
        except Exception as e:
            messagebox.showerror("Error", f"No se pudo mostrar la imagen: {str(e)}")

    def delete_image(self):
        # Obtener la imagen seleccionada
        try:
            index = self.image_list.curselection()[0]
            image_name = self.image_list.get(index)
        except IndexError:
            messagebox.showerror("Error", "Selecciona una imagen primero.")
            return

        # Confirmar eliminación
        response = messagebox.askyesno("Eliminar", f"¿Estás seguro que deseas eliminar '{image_name}'?")
        if response == tk.YES:
            # Eliminar la imagen del directorio
            id_old = self.current_directory.split("_", -1)[1]
            try:
                my_DB.write_DB(tabla='users', parametros=['n_im'], valores=['n_im-1'], filtro=int(id_old))
            except:
                print(self.current_directory.split("_", -1))
                messagebox.showerror("Error", f"No se pudo establecer conexion con la DB")
                return
            try:
                image_path = os.path.join(self.current_directory, image_name)
                os.remove(image_path)
                messagebox.showinfo("Eliminado", f"La imagen '{image_name}' ha sido eliminada correctamente.")
                # Actualizar la lista después de eliminar
                self.update_image_list()
            except Exception as e:
                messagebox.showerror("Error", f"No se pudo eliminar la imagen: {str(e)}")

    def move_image(self):
        # Obtener la imagen seleccionada
        try:
            index = self.image_list.curselection()[0]
            image_name = self.image_list.get(index)
        except IndexError:
            messagebox.showerror("Error", "Selecciona una imagen primero.")
            return

        # Elegir el nuevo directorio de destino
        destination = filedialog.askdirectory(initialdir="/home/usuario/Project/Faces/", title="Selecciona el directorio de destino")
        if not destination:
            return

        # Mover la imagen al nuevo directorio
        try:
            source_path = os.path.join(self.current_directory, image_name)
            destination_path = os.path.join(destination, image_name)
            id_old = self.current_directory.split("_", -1)[1]
            id_new = destination.split("_", -1)[1]
            try:
                my_DB.write_DB(tabla='users', parametros=['n_im'], valores=['n_im-1'], filtro=int(id_old))
                my_DB.write_DB(tabla='users', parametros=['n_im'], valores=['n_im+1'], filtro=int(id_new))
            except:
                print(self.current_directory.split("_", -1))
                messagebox.showerror("Error", f"No se pudo establecer conexion con la DB")
                return
            shutil.move(source_path, destination_path)
            messagebox.showinfo("Movido", f"La imagen '{image_name}' ha sido movida correctamente.")
            # Actualizar la lista después de mover
            self.update_image_list()
        except Exception as e:
            messagebox.showerror("Error", f"No se pudo mover la imagen: {str(e)}")

if __name__ == "__main__":
    root = tk.Tk()
    app = ImageManager(root)
    root.mainloop()
