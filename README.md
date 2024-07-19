# Embedded-face-recognition-system
El repositiorio presenta los codigos fuentes asi como instalación de un sistema de reconocimeinto facial para la Raspberry Pi con el objetivo de gestionar el acceso a un determinado activo. El sistema, entrenará de manera automatica un clasificador para completar el reconocimiento facial , mientras tanto la autentificación será mediante PIN . El sistema requiere del uso de un nodo front-end ( la Raspberry Pi) y un back-end (el computador). En el back-end, estará corriendo un servico de MySQL para alamcenar la informacion de los activos y usuario. El funcionamiento esta basado en el OS de Ubutnu 22.04 para ambos nodos.Se han dividido los codigos en dos directorios cada uno correspondiente a cada nodo.


# Instalacion previa:
## Front-end
Necesita instalar las siguientes librerias:
*OpenCV, versión 4.9.0 [Sitio web Opencv](https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html)
*Frugally Deep, versión 1.15.0 [Sitio web Frugally Deep](https://github.com/Dobiasd/frugally-deep)
*Dlib, versión 19.9 [Sitio web Dlib](http://dlib.net/)
*Gpiod, versión 1.6.3
*Myslqcppconn, versión 1.1.13

## Back-end
Necesita instalar las siguientes librerias:
*OpenCV, versión 4.9.0 [sitio web Opencv](https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html)
*Frugally Deep, versión 1.15.0 [sitio web Frugally Deep](https://github.com/Dobiasd/frugally-deep)
*Dlib, versión 19.9 [sitio web Dlib](http://dlib.net/)
*Keras, version [sitio web Keras](1.15.0 https://keras.io/)
*Mysqlconnector, versión 1.1.13

# Puesta en Marcha
*Descargue y compile los codigos correspondientes al front-end, se deberá alamcenar los datos en la carpeta con la misma estructura que en el repositorio.
*Descargue el directorio del repositorio de back-end.
*Ejecute los .py del bakc-end, sock_mult y launch-training. Si esta probando la version 2 deberá ejecutar el archivo encode.py .
*Ejecute uno de los Face_recog, v1 o v2 compilados.

