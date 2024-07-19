//============================================================================
// Name        :
// Author      : Armando Bond
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <stdlib.h>
#include <iostream>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <memory>
#include <filesystem>
#include <chrono>

#include <mysql_connection.h>
#include <mysql_driver.h>
#include <mysql_error.h>
#include <cppconn/build_config.h>
#include <cppconn/config.h>
#include <cppconn/connection.h>
#include <cppconn/datatype.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/metadata.h>
#include <cppconn/parameter_metadata.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/resultset_metadata.h>
#include <cppconn/statement.h>
#include <cppconn/sqlstring.h>
#include <cppconn/warning.h>
#include <cppconn/version_info.h>
#include <cppconn/variant.h>





#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <random>
#include <fdeep/fdeep.hpp>
#include <stdio.h>
#include <dlib/dnn.h>
#include <dlib/gui_widgets.h>
#include <dlib/clustering.h>
#include <dlib/string.h>
#include <dlib/image_io.h>
#include <dlib/opencv.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <gpiod.hpp>


#define PORT_JSON 5003
#define PORT_MULT 6000

#define FACE_FOLDER "/home/user/Project/all_faces"
#define FACE_FOLDER_AUX "/home/user/Project/cam_sim" ////////////////////////////////
#define MODEL_PATH "/home/user/Project/Model/fdeep_model.json"
#define MIN_TRAIN_NIMG 40
#define FACE_CASCADE_PATH "/home/user/Project/haarcascade_frontalface_default.xml"
#define FACE_ENCODINGS_PATH "/home/user/Project/dlib_face_recognition_resnet_model_v1.dat"
#define FACE_SHAPE_PATH "/home/user/Project/shape_predictor_68_face_landmarks.dat"
bool reset=false,flag_load_model=true;
int rutina;
std::unique_ptr<fdeep::model> model;
cv::CascadeClassifier face_cascade(FACE_CASCADE_PATH);
using namespace sql;
using namespace std;
using namespace dlib;
using namespace gpiod;
namespace fs=std::filesystem;



template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual = add_prev1<block<N,BN,1,tag1<SUBNET>>>;

template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual_down = add_prev2<avg_pool<2,2,2,2,skip1<tag2<block<N,BN,2,tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET>
using block  = BN<con<N,3,3,1,1,relu<BN<con<N,3,3,stride,stride,SUBNET>>>>>;

template <int N, typename SUBNET> using ares      = relu<residual<block,N,affine,SUBNET>>;
template <int N, typename SUBNET> using ares_down = relu<residual_down<block,N,affine,SUBNET>>;

template <typename SUBNET> using alevel0 = ares_down<256,SUBNET>;
template <typename SUBNET> using alevel1 = ares<256,ares<256,ares_down<256,SUBNET>>>;
template <typename SUBNET> using alevel2 = ares<128,ares<128,ares_down<128,SUBNET>>>;
template <typename SUBNET> using alevel3 = ares<64,ares<64,ares<64,ares_down<64,SUBNET>>>>;
template <typename SUBNET> using alevel4 = ares<32,ares<32,ares<32,SUBNET>>>;

using anet_type = loss_metric<fc_no_bias<128,avg_pool_everything<
                            alevel0<
                            alevel1<
                            alevel2<
                            alevel3<
                            alevel4<
                            max_pool<3,3,2,2,relu<affine<con<32,7,7,2,2,
                            input_rgb_image_sized<150>
                            >>>>>>>>>>>>;


cv::Mat rotation(cv::Mat img,int ang){
	cv::Mat out_img;
	double angle= std::rand()% (ang*2+1) + (-ang);
	int rows = img.rows;
	int cols = img.cols;
	cv::Size s = img.size();
	rows = s.height;
	cols = s.width;

	cv::Mat rotMat=cv::getRotationMatrix2D(cv::Point2f(cols/2,rows/2),angle,1);
	cv::warpAffine(img,out_img,rotMat,cv::Size(cols,rows) );
	return out_img;
}

cv::Mat horizontal_flip(cv::Mat img){
	if ((std::rand()% 2)==1){
		cv::flip(img,img,1);
		return img;
	}
	else{
		return img;
	}
}


cv::Mat convertFaceChipToMat(const dlib::matrix<dlib::rgb_pixel>& face_chip) {
    // Crear una imagen de OpenCV con el mismo tamaño que el face_chip
    cv::Mat img(face_chip.nr(), face_chip.nc(), CV_8UC3);

    // Copiar los datos de face_chip a la imagen de OpenCV
    for (long r = 0; r < face_chip.nr(); ++r) {
        for (long c = 0; c < face_chip.nc(); ++c) {
            dlib::rgb_pixel pixel = face_chip(r, c);
            img.at<cv::Vec3b>(r, c) = cv::Vec3b(pixel.blue, pixel.green, pixel.red);  // OpenCV usa el orden BGR
        }
    }

    return img;
}

class MySocket {
	public:
	int port_train;
	int port_send;
	int port_json;
	int port_enc=6001;
	const char* host;
	const char* my_ip;
	int buff_size;
	void init_con(){
		char buffer[(*this).buff_size];
		int clientSocket = socket(AF_INET,SOCK_STREAM,0);
		// specify address
		sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(PORT_MULT);
		serverAddress.sin_addr.s_addr =inet_addr((* this).host);

		// sending connection request
		connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
		send(clientSocket, "Hola servidor", strlen("Hola servidor"), 0);
		read(clientSocket,buffer,(* this).buff_size-1);
		char *ptr;
		ptr=strtok(buffer,"<>");
		while(ptr!=NULL){
			if (strcmp(ptr,"IMG")==0){
				ptr = strtok(NULL,"<>");
				(* this).port_send=stoi(ptr);
			}
			else if(strcmp(ptr,"TRAIN")==0){
				ptr = strtok(NULL,"<>");
				(* this).port_train=stoi(ptr);
			}
			ptr = strtok(NULL,"<>");
		}
		close(clientSocket);
	}
	void launch_training(){
		// creating socket
		int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		// specifying address
		sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons((* this).port_train);
		serverAddress.sin_addr.s_addr =inet_addr((* this).host);

		// sending connection request
		connect(clientSocket, (struct sockaddr*)&serverAddress,sizeof(serverAddress));

		// sending data
		const char* message = "15   Launch Training";
		send(clientSocket, message, strlen(message), 0);

		// closing socket
		close(clientSocket);
	}
	void send_image(const char* filename,int id,const char* folder_inf){
		int valread;
		char buffer[(*this).buff_size];
		const char* message;
		const char* bytes_read;
		bool Successful = false;
		// creating socket
		int clientSocket = socket(AF_INET,SOCK_STREAM,0);

		// specify address
		sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons((* this).port_send);
		serverAddress.sin_addr.s_addr =inet_addr((* this).host);

		// sending connection request
		connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
		// sending data
		FILE *myfile = NULL;
		myfile = fopen(filename,"rb");
		fseek(myfile,0,SEEK_END);
    	int filesize = ftell(myfile);
    	string full_msg = "INFO<>" +  to_string(id) + "<>"+ folder_inf + "<>" + filename + "<>" + to_string(filesize) + "<>None";
		message = full_msg.c_str();
		send(clientSocket, message, strlen(message), 0);
		valread = read(clientSocket,buffer,(* this).buff_size-1);
		if(strcmp((const char*)buffer,"INFO RECV")==4){
			cout<< buffer <<endl;
		}
		fseek(myfile,0,SEEK_SET);
		while (!feof(myfile)){
			fread(buffer,sizeof(buffer),1,myfile);
			//cout<<buffer;
			if (buffer==0){
				cout<< "Estamos en el if";
				Successful=true;
				fclose(myfile);
				break;
			}
			send(clientSocket, buffer, (* this).buff_size, 0);
		}
		close(clientSocket);
	}

	void recv_json(bool * flag){
		char buffer[(*this).buff_size];
		int serverSocket = socket(AF_INET,SOCK_STREAM,0);

		// specify address
		sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons((* this).port_json);
		serverAddress.sin_addr.s_addr =inet_addr((* this).my_ip);

		bind(serverSocket, (struct sockaddr*)&serverAddress,sizeof(serverAddress));

		listen(serverSocket,5);
		while (true){
			int clientSocket = accept(serverSocket, nullptr, nullptr);

			int valread = read(clientSocket,buffer,(* this).buff_size-1);
			cout<< valread;
			std::vector <std::string> seglist;
			std::string segment;
			char *ptr;
			ptr=strtok(buffer,"<>");
			int filesize;
			while(ptr!=NULL){
				if (strcmp(ptr,"FILENAME")==0){
					ptr = strtok(NULL,"<>");
				}
				else if(strcmp(ptr,"FILESIZE")==0){
					ptr = strtok(NULL,"<>");
					filesize=stoi(ptr);
				}
				ptr = strtok(NULL,"<>");
			}
			cout<<filesize<<endl;

			FILE *myfile = NULL;
			myfile = fopen(MODEL_PATH,"wb");
			cout<<"a"<<endl;
			char msg[200];
			int bytes_read;
			while((bytes_read=recv(clientSocket,msg,200,0))>0){
				fwrite(msg,1,bytes_read,myfile);
			}
			fclose(myfile);
			*flag=true;
		}
		close(serverSocket);
	}

	void recv_enc(matrix<rgb_pixel> *face_chip, std::vector <float> *emb_face ){
		    // Enviar la imagen (convertir matrix<rgb_pixel> a cv::Mat)
		    cout<<"Realizar convert dlib to cv"<<endl;
		    cv::Mat sendim = convertFaceChipToMat(*face_chip);
		    cout<<"Conversion hecha"<<endl;

		    std::vector<uchar> imgVector;
		    cout<<"If pocho"<<endl;
		    if (sendim.isContinuous()) {
		    	cout<<"If pocho 1"<<endl;
		        imgVector.assign(sendim.datastart, sendim.dataend);
		    } else {
		    	cout<<"If pocho 2"<<endl;
		        for (int i = 0; i < sendim.rows; ++i) {
		            imgVector.insert(imgVector.end(), sendim.ptr<uchar>(i), sendim.ptr<uchar>(i) + sendim.cols * sendim.channels());
		        }
		    }



		    const char* SERVER_IP = this->host;
		    const int SERVER_PORT = 6001;

		    // Crear el socket
		        int sock = socket(AF_INET, SOCK_STREAM, 0);
		        if (sock < 0) {
		            std::cerr << "Error al crear el socket." << std::endl;
		            return;
		        }

		        // Configurar la dirección del servidor
		        struct sockaddr_in server_addr;
		        memset(&server_addr, 0, sizeof(server_addr));
		        server_addr.sin_family = AF_INET;
		        server_addr.sin_port = htons(SERVER_PORT);
		        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
		        cout<<"Socket Conf"<<endl;
		        // Conectar al servidor
		        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		            std::cerr << "Error al conectar al servidor." << std::endl;
		            close(sock);
		            return;
		        }
		        cout<<"Socket Conn"<<endl;
		        // Enviar el tamaño del vector
		        int32_t imgSize = htonl(imgVector.size());
		        send(sock, &imgSize, sizeof(imgSize), 0);
		        cout<<"Img Ize send"<<endl;
		        // Enviar la imagen
		        send(sock, imgVector.data(), imgVector.size(), 0);
		        cout<<"IMg Send"<<endl;
		        // Recibir el vector de 128 valores
		        std::vector<float> receivedVector(128);
		        ssize_t totalReceived = 0;
		        ssize_t bytesToReceive = receivedVector.size() * sizeof(float);

		        while (totalReceived < bytesToReceive) {
		            ssize_t received = recv(sock, (char*)receivedVector.data() + totalReceived, bytesToReceive - totalReceived, 0);
		            if (received <= 0) {
		                std::cerr << "Error al recibir los datos." << std::endl;
		                close(sock);
		                return;
		            }
		            totalReceived += received;
		        }
		        *emb_face=receivedVector;
		        cout<<"Vec Recv"<<endl;


		        // Imprimir los valores recibidos
		        /*
		        for (float val : *emb_face) {
		            std::cout << val << " ";
		        }
		       	std::cout << std::endl;
		        */
		        return;
	}
};

struct Salidas_DB{
	string v1;
	std::vector <string> v2;
	string v3;
	string v4;
};

class MySqlClass{
public:
	const char* server;
	const char* username;
	const char* password;
	const char* DB_used;
	struct Salidas_DB output;
	void read_DB(const string tabla,const string parametro,const string valor){
		sql::Driver *driver;
		sql::Connection *con;
		sql::PreparedStatement *pstmt;
		sql::ResultSet *result;

		try
		{
			driver = get_driver_instance();
			//for demonstration only. never save password in the code!
			con = driver->connect(this->server, this->username, this->password);
		}
		catch (sql::SQLException e)
		{
			cout << "Could not connect to server. Error message: " << e.what() << endl;
			exit(1);
		}

		con->setSchema(this->DB_used);
		if(tabla == "servicios")
			pstmt = con->prepareStatement("SELECT " + parametro + " FROM " + tabla + " WHERE servicio=\""+ valor +"\";");
		else
			pstmt = con->prepareStatement("SELECT " + parametro + " FROM " + tabla + ";");


		result = pstmt->executeQuery();


		while (result->next()){
			//printf("Reading from %s = %d\n",tabla.c_str(),result->getInt(1) , result->getInt(2));
			if (tabla=="servicios"){
				this->output.v1=result->getString(1);
			}
			else if(tabla=="users"){
				this->output.v2.push_back(result->getString(1));
				//this->output.v1=result->getString(1);
				//this->output.v2=result->getString(2);
				//this->output.v3=result->getString(3);
				//this->output.v4=result->getString(4);
			}

		}
		delete result;
		delete pstmt;
		delete con;
	}

	void write_DB(const string tabla, const string parametro, const string valor, const string id){
		sql::Driver *driver;
		sql::Connection *con;
		sql::PreparedStatement *pstmt;
		sql::ResultSet *result;
		try
		{
			driver = get_driver_instance();
			//for demonstration only. never save password in the code!
			con = driver->connect(this->server, this->username, this->password);
		}
		catch (sql::SQLException e)
		{
			cout << "Could not connect to server. Error message: " << e.what() << endl;
			exit(1);
		}
		con->setSchema(this->DB_used);
		if (tabla == "users")
			pstmt = con->prepareStatement("UPDATE " + tabla + " SET " + parametro + "=" + valor + " WHERE id="+ id +";");
		else if (tabla == "servicios")
				pstmt = con->prepareStatement("UPDATE " + tabla + " SET " + parametro + "=" + valor+ "WHERE servicio="+id+";");
		else{
			printf("Error al actualizar valores");
			exit(1);
		}

		pstmt->executeQuery();

		delete pstmt;
		delete con;
	}

};

class Teclado{
	private:
		char key[4][4] ={
				{'1','2','3','A'},
				{'4','5','6','B'},
				{'7','8','9','C'},
				{'*','0','#','D'}
		};
		int c,f;
		gpiod::chip chip;
		gpiod::line line_in[4];
		gpiod::line line_out[4];
	public:

	void initialize(){
		this->chip = gpiod::chip("gpiochip0");
		unsigned int line_num_out[4] = {17 , 27, 22, 23};
		unsigned int line_num_in[4] = {16 , 26, 6, 5};
		if(!this->chip){
			cout<<"Fallo al abrir el chip"<<endl;
			return;
		}

		for(int i=0;i<4;i++){
			this->line_in[i] = gpiod::line(this->chip.get_line(line_num_in[i]));
			if(!this->line_in[i]){
				cout<<"Error al acceder a la linea IN "<< i<<endl;
			} else{
				gpiod::line_request config{};
				config.consumer="Teclado_in";
				config.request_type = gpiod::line_request::DIRECTION_INPUT;
				config.flags = gpiod::line_request::FLAG_BIAS_PULL_UP;
				this->line_in[i].request(config);
			}
		}

		for(int i=0;i<4;i++){
			this->line_out[i] = gpiod::line(this->chip.get_line(line_num_out[i]));
			if(!this->line_out[i]){
				cout<<"Error al acceder a la linea OUT "<< i<<endl;
			} else{
				gpiod::line_request config{};
				config.consumer="Teclado_out";
				config.request_type = gpiod::line_request::DIRECTION_OUTPUT;

				this->line_out[i].request(config);
				this->line_out[i].set_value(1);
			}
		}
	}

	bool read_teclado(){
		bool tecla= false;
		int timeoutMS=10000;
		int miniT=1000;
		int nveces = timeoutMS * 1000/miniT;
		for(int k=0;k<nveces;k++){
			for(int i=0;i<4;i++){
				this->line_out[i].set_value(0);
				for(int j=0;j<4;j++){
					if(!this->line_in[j].get_value()){
						while(!this->line_in[j].get_value()){
							usleep(miniT);
						}
						tecla=true;
						this->c=i;
						this->f=j;
						cout<<"pulsaste: "<< this->key[c][f]<<endl;
						this->line_out[i].set_value(1);
						return tecla;
					}
				}
				usleep(miniT);
				this->line_out[i].set_value(1);
			}
		}
		return tecla;
	}

	string leer_pin(){
		string pin;
		while(true){
			if(this->read_teclado()){
				char key_pul= this->key[this->c][this->f];
				if(key_pul == '*'){
					cout<<"Pin recibido: "<<pin<<endl;
					return  pin;
				}
				else if (key_pul == '#'){
					if(!pin.empty()){
						pin.pop_back();
					}
				}
				else {
					if(pin.length()<4){
						pin+= key_pul;
					}
				}
				cout<<"Pin: "<<pin<<endl;
			}
		}
	}

	string leer_servicio(){
		string Mx = "M";
		while(true){
			if(this->read_teclado()){
				cout<<"C: "<<this->c<<" F: "<<this->f<<endl;
				if(this->f == 3 && Mx.length()<2){
					Mx += to_string(this->c+1);
					cout<<"Mx actual: " <<Mx<<endl;
				}
				else if (this->f == 0 && this->c == 3){
					cout<<"Mx introducido: "<< Mx<<endl;
					return Mx;
				}
				else if(this->f == 2 && this->c == 3 && Mx.length()>1){
					Mx.pop_back();
					cout<<"Mx actual: "<<Mx<<endl;
				}

			}
		}
	}
};

class MyLed{
	private:
		gpiod::chip chip;
		gpiod::line line_out[2];

	public:

	void initialize(){
		this->chip = gpiod::chip("gpiochip0");
		unsigned int line_num_out[2] = {24 , 25};
		cout<<"Estoy iniciando leds";
		if(!this->chip){
			cout<<"Fallo al abrir el chip"<<endl;
			return;
		}

		for(int i=0;i<2;i++){
			this->line_out[i] = gpiod::line(this->chip.get_line(line_num_out[i]));
			if(!this->line_out[i]){
				cout<<"Error al acceder a la linea OUT "<< i<<endl;
			} else{
				gpiod::line_request config{};
				config.consumer="Leds_out";
				config.request_type = gpiod::line_request::DIRECTION_OUTPUT;
				this->line_out[i].request(config);
				this->line_out[i].set_value(0);
			}
		}
	}

	void set_led_value(int led,int value){
		this->line_out[led].set_value(value);
			return;
		}
	void led_toggle(int led, int ms){
		this->set_led_value(led,1);
		usleep(ms*1000);
		this->set_led_value(led,0);
	}
};

int capture_img(string path, string name){
		cv::VideoCapture cap;//0,cv::CAP_V4L
			cap.open(0,cv::CAP_V4L2);
			if (!cap.isOpened()){
				cout<<"Error al abrir la cámara"<<endl;
				return -1;
			}
			cap.set(cv::CAP_PROP_FRAME_WIDTH,1280);
			cap.set(cv::CAP_PROP_FRAME_HEIGHT,720);
			usleep(2000000);
			while (true){
				cv::Mat frame;
				//cap.read(frame);
				cap>>frame;
				if (frame.empty()){
					cout<<"NO veo nada"<<endl;
					return -1;
				}
				else{
					cout<<"Ta bien"<<endl;
					cv::imwrite(path+name,frame);
					cap.release();
					return 0;
				}
			}

	}

void permiso(MySqlClass *mydb,MyLed *leds,int id, string servicio){
	mydb->read_DB("servicios","permisos",servicio);
	string perm = mydb->output.v1;
	if(perm[id]=='1'){
		cout<<"Permiso concedido servicio ocupado"<<endl;
		//mydb->write_DB("servicios","current_tokens","current_tokens+1",servicio);
		leds->led_toggle(1,2*1000);
	}
	else{
		cout<<"Usuario sin permiso de acceso"<<endl;
		leds->led_toggle(0,2*1000);
	}
	return;
}

void thread_train(MySocket *sock){
	sock->launch_training();
}
void server_json(MySocket *sock,bool *flag){
	sock->recv_json(flag);
}
void upload_image(MySocket *sck, MySqlClass *mydb, cv::Mat crop_face,string file_name, int id){
	string folder_inf = "Train";
	int rand_num=std::rand()%100;
	if(rand_num<15){
		cout<<"Sending data Val";
		folder_inf="Val";
		sck->send_image((const char*)file_name.c_str(),id,(const char*)folder_inf.c_str());
	}
	else{
		cout<<"Sending data Train";
		sck->send_image((const char*)file_name.c_str(),id,(const char*)folder_inf.c_str());
		mydb->write_DB("users","n_im","n_im +1",std::to_string(id));
		mydb->write_DB("users","last_im","CURRENT_TIMESTAMP()",std::to_string(id));
	}
	//cout<<"Sending data";
	//sck->send_image((const char*)file_name.c_str(),id);
	cout<<"Primera img";
	//mydb->write_DB("users","n_im","n_im +1",std::to_string(id));
	//mydb->write_DB("users","last_im","CURRENT_TIMESTAMP()",std::to_string(id));
	remove(file_name.c_str());

	for(int n_img_gen=0;n_img_gen<4;n_img_gen++){
		cv::Mat img_aug=rotation(crop_face,15);
		img_aug=horizontal_flip(img_aug);
		string file_name_aug=file_name.substr(0,file_name.size()-5)+"-"+to_string(n_img_gen)+"-"+".jpeg";
		cv::imwrite(file_name_aug,img_aug);
		cout<< file_name_aug<<endl;
		sck->send_image((const char*)file_name_aug.c_str(),id,(const char*)folder_inf.c_str());
		remove(file_name_aug.c_str());
		if(folder_inf=="Train"){
			mydb->write_DB("users","n_im","n_im +1",std::to_string(id));
			mydb->write_DB("users","last_im","CURRENT_TIMESTAMP()",std::to_string(id));

		}
		//mydb->write_DB("users","n_im","n_im +1",std::to_string(id));
		//mydb->write_DB("users","last_im","CURRENT_TIMESTAMP()",std::to_string(id));
	}

}
bool test_model_exist(){
	if(FILE *file= fopen(MODEL_PATH,"r")){
		fclose(file);
		return true;
	}
	else{
		return false;
	}
}
int main()
{
	MyLed myleds;
	myleds.initialize();
	cout<<"Leds iniciados"<<endl;
	try{
	cout<<"Hola Mondo"<<endl;
	//Configuración del socket
	std::ifstream conn_data("/home/user/Project/conn_data.txt");
	if(!conn_data){
		cerr<< "No hay archivo de configuración de conexiones"<<endl;
		return -1;
	}

	cout<<"Aun no exploté"<<endl;
	std::string HOST_IP,MY_IP,MYSQL_SERVER,MYSQL_USER,MYSQL_PASSWORD;
	conn_data>>HOST_IP>>MY_IP>>MYSQL_SERVER>>MYSQL_USER>>MYSQL_PASSWORD;

	cout<<HOST_IP<<endl;
	cout<<MY_IP<<endl;
	cout<<MYSQL_SERVER<<endl;
	cout<<MYSQL_USER<<endl;
	cout<<MYSQL_PASSWORD<<endl;

	conn_data.close();
	cout<<"Archivo Config Leido"<<endl;
	MySocket sck;
	sck.port_json=PORT_JSON;
	sck.host=HOST_IP.c_str();
	sck.my_ip=MY_IP.c_str();
	sck.buff_size=4096;
	sck.init_con();
	cout<<"Socket inicializado"<<endl;
	std::thread serv_fdeep_model(server_json,&sck,&flag_load_model);
	serv_fdeep_model.detach();
	cout<<"Hilo recibir modelo iniciado"<<endl;

	//Configuración DB
	MySqlClass mydb;
	mydb.DB_used = "usuarios";
	mydb.server = MYSQL_SERVER.c_str();
	mydb.username = MYSQL_USER.c_str();
	mydb.password = MYSQL_PASSWORD.c_str();
	//mydb.read_DB("servicios","M1");
	cout<<"DB inicializada"<<endl;
	Teclado mytec;
	mytec.initialize();
	cout<<"Teclado iniciado"<<endl;

	anet_type net;
	deserialize(FACE_ENCODINGS_PATH) >> net;
	dlib::shape_predictor sp;
	deserialize(FACE_SHAPE_PATH) >>sp;
	string save_im_path="/home/user/capturas/";
	string save_im_name="img_";
	char save_im_codec= 'a';
	int  save_im_num=0;
	//mydb.write_DB("servicios","M1", "0", "0");
	//printf("\n%d %d",stoi(mydb.output.v1),stoi(mydb.output.v2));
	cout<<"Empieza el programa"<<endl;
	while(1){
		cout<<"Selecciona un servicio"<<endl;
		//Seleccionar Servicio
		string Mx = mytec.leer_servicio();
		cout<< "Has introducido : " << Mx <<endl;

		try{
			mydb.read_DB("servicios","current_tokens",Mx);
		}
		catch(sql::SQLException e){
			cout<<"Error al seleccionar servicio\n";
			continue;
		}
		if (mydb.output.v1 == "0"){
			cout<<"Servicio libre\n";
			myleds.led_toggle(1,2*1000);
		}
		else{
			cout<<"Servicio ocupado\n";
			myleds.led_toggle(0,2*1000);
			continue;
		}

		//Procesa la imagen para obtener una cara recortada en escal de grises
		cout<<"Captura y procesado de imagen"<<endl;
		cv::Mat image;
		cv::Mat gray;
		cv::Mat crop_face;
		std::vector<cv::Rect> faces;
		string file_name;
		int num_int=0;

		while(1){ //Comprueba que se ha detectado al menos una cara en la imagen
			//Captura de Imagen por camara//
			//Captura de Imagen por camara//
			std::vector<std::string> imagePaths;
			for (const auto& entry : fs::directory_iterator(FACE_FOLDER_AUX)){
				if (entry.path().extension()== ".jpeg"){
					imagePaths.push_back(entry.path());
				}
			}
			int rnd_index=std::rand() % (int)imagePaths.size();
			file_name=imagePaths[rnd_index];

			///////////////////////////////////
			string save_im_true_name=save_im_name+save_im_codec+std::to_string(save_im_num)+".jpeg";
			cout<<"Sacando foto en: 3..."<<endl;
			usleep(1*1000*1000);
			cout<<"Sacando foto en: 2..."<<endl;
			usleep(1*1000*1000);
			cout<<"Sacando foto en: 1..."<<endl;
			usleep(1*1000*1000);
			if(capture_img(save_im_path,save_im_true_name)!=-1){
			//if(true){
				save_im_num++;
				if (save_im_num>300){
					save_im_num=0;
					if (save_im_codec=='z'){
						save_im_codec='a';
					}
					else{
						save_im_codec=(char)(((int)save_im_codec)+1);
					}
				}
			}
			else{
				reset=true;
				break;
			}
			///////////////////////////////////
			/*std::filesystem
			std::ifstream sim_im(file_name, std::ios::binary);
			std::ofstream dest (save_im_path+save_im_true_name,std::ios::binary);
			dest << sim_im.rdbuf();
			//std::filesystem::copy(file_name,save_im_path+save_im_true_name);
			*/
			///////////////////////////////
			image= cv::imread(save_im_path+save_im_true_name);
			cout<<"Abriendo imagen"<<endl;
			file_name=save_im_path+save_im_true_name;
			cv::cvtColor(image,gray,cv::COLOR_RGB2GRAY);
			face_cascade.detectMultiScale(gray,faces,1.1,6,cv::CASCADE_FIND_BIGGEST_OBJECT,cv::Size(200,200));
			if(faces.size()<=0){ //Si no se detecta cara repite el proceso de captura y detección
				cout<<"No detecto cara"<<endl;
				if(num_int>=3){ //Si se superan 3 intentos de capturar la cara reinicia el bucle principal en fuera del while
					cout<<"Reiniciando operación"<<endl;
					num_int=0;
					remove(file_name.c_str());
					reset=true;
					myleds.led_toggle(0,2*1000);
					break;
				}
				else{
					cout<<"Se vuelve a capturar la imagen"<<endl;
					num_int++;
					remove(file_name.c_str());
					myleds.led_toggle(0,750);
					usleep(500*1000);
					myleds.led_toggle(0,750);
					usleep(500*1000);
					myleds.led_toggle(0,750);
					continue;
				}
			}
			else{ //Se ha detectado cara se sale del bulce actual
				cout<<"Cara detectada"<<endl;
				num_int=0;
				myleds.led_toggle(1,750);
				usleep(500*1000);
				myleds.led_toggle(1,750);
				usleep(500*1000);
				myleds.led_toggle(1,750);
				break;
			}
		}
		if (reset==true){ //SE habian superado el limite de intentos resetea el codigo
			reset=false;
			continue;
		}
		//Filtrar por tamaño
		cout<<"Filtrando por tamaño"<<endl;
		cv::Rect large_face;
		for(size_t i = 0; i < faces.size();i++){
			if(faces[i].area() > large_face.area()){
				large_face = faces[i];
			}
		}
		cout<<"Cara filtrada"<<endl;
		crop_face=image(large_face); //Se exporta la imagen en niveles de gris
		cv::imwrite(file_name,crop_face);
		///*
		cv::imshow("Face",crop_face);
		cv::waitKey(0);
		//*///
		cout<<file_name<<endl;
		cout<<"Foto modificada guardada"<<endl;

		//Analizar si estamos en condicion de reconocer o registrar caras
		if (test_model_exist()){ //Se debería añadir algo para que si el modelo no ha sido actualizado no lo vuelva a cargar
		//Cargar modelo
			if (flag_load_model==true){
				cout<< "Modelo existente, se procede a cargarlo" <<endl;
				//const auto model = fdeep::load_model(MODEL_PATH);
				 const auto model_frg =  fdeep::load_model(MODEL_PATH);
				 model = std::make_unique<fdeep::model>(model_frg);
				 flag_load_model=false;
			}
			//
		//Realiza el reconocimiento
			//Termina de procesar la imagen:
			cout<<"Realizando reconocimiento Facial"<<endl;

	    	cv::Mat img_resized;
	    	cv::resize(crop_face,img_resized,cv::Size(150,150),cv::INTER_LINEAR);
	    	cout<<"Resized 150x150"<<endl;
	    	//Obtine el vector de 128 valores que define la cara
	    	dlib::matrix<dlib::rgb_pixel> img_net;
	    	std::vector<dlib::matrix<dlib::rgb_pixel>> emb_face_dlib;
	    	dlib::cv_image<dlib::rgb_pixel>dlib_img(img_resized);
	    	dlib::rectangle dlib_rect(0,0,img_resized.cols,img_resized.rows);

	    	dlib::full_object_detection shape = sp(dlib_img,dlib_rect);
	    	matrix<rgb_pixel> face_chip;
	    	dlib::extract_image_chip(dlib_img,get_face_chip_details(shape,150,0.25),face_chip);
	    	cout<<"Face aligment"<<endl;

	    	//dlib::assign_image(img_net, dlib::cv_image<dlib::rgb_pixel>(img_resized));
	    	//dlib::assign_image(img_net, );
	    	//emb_face_dlib.push_back(img_net);
	    	emb_face_dlib.push_back(face_chip);
	    	auto start_vec = std::chrono::high_resolution_clock::now();
	    	std::vector<dlib::matrix<float,0,1>> face_descriptors = net(emb_face_dlib);
	    	auto stop_vec = std::chrono::high_resolution_clock::now();
	    	emb_face_dlib.clear();
	    	std::chrono::duration<double> duration_vec = stop_vec - start_vec;
	    	cout<<"Vector extraido en: "<< duration_vec.count() <<endl;
	    	std::vector <float> emb_face;
	        for (auto it = face_descriptors[0].begin();it != face_descriptors[0].end(); ++it){
	             emb_face.push_back(*it);
	    	}

			std::pair<std::size_t, double> pred;
	    	auto start_pred = std::chrono::high_resolution_clock::now();
			pred = model->predict_class_with_confidence({fdeep::tensor(fdeep::tensor_shape(static_cast<std::size_t>(128)),emb_face)});
	    	auto stop_pred = std::chrono::high_resolution_clock::now();
	    	std::chrono::duration<double> duration_pred = stop_pred - start_pred;
			emb_face.clear();
			cout<<"Clasificación en: "<< duration_pred.count()<<endl;
			if (pred.second >=0.9){
				myleds.set_led_value(1,1);
				myleds.set_led_value(0,1);
				usleep(2*1000*1000);
				myleds.set_led_value(1,0);
				myleds.set_led_value(0,0);
				cout<<"Cara reconocida"<<endl;
				cout<<"ID: "<<pred.first<<endl;
				cout<<"Confidence: " << pred.second<<endl;
				//Borra la imagen adquirida
				remove(file_name.c_str());
				permiso(&mydb,&myleds,pred.first,Mx);/////////////////////////////
				continue;//////////////////////////////

			}
			else{
				cout<<"Cara no reconocida"<<endl;
				rutina=2;
			}
		}
		else{
			cout<< "Modelo no existente" <<endl;
			rutina=2;
		}


		//Rutina de registro
		if(rutina==2){
			rutina=0;
			cout<<"Cara no conocida"<<endl;
			mydb.output.v2.clear();
			mydb.read_DB("users","pin","NONE");
			cout<<mydb.output.v1 <<endl;
			myleds.set_led_value(1,0);
			myleds.set_led_value(0,1);
			usleep(500*1000);
			myleds.set_led_value(1,1);
			myleds.set_led_value(0,0);
			usleep(500*1000);
			myleds.set_led_value(1,0);
			myleds.set_led_value(0,1);
			usleep(500*1000);
			myleds.set_led_value(1,0);
			myleds.set_led_value(0,1);
			usleep(500*1000);
			myleds.set_led_value(1,0);
			myleds.set_led_value(0,0);
			while(true){
				cout<<"Esperando PIN"<<endl;
				string pin = mytec.leer_pin();
				cout<< "Pin introducido: "<<pin<<endl;
				bool confirm_access=false;
				int id=0;
				for(std::vector<string>::iterator pin_db=mydb.output.v2.begin(); pin_db !=mydb.output.v2.end();pin_db++){
					cout<<"Pin esperado "<<pin_db->c_str()<<endl;
					if(strcmp((const char*)pin.c_str(),(const char*)pin_db->c_str())==0){
						cout<<"Pin correcto"<<endl;
						num_int=0;
						confirm_access=true;

					}
					else if(pin_db == mydb.output.v2.end()-1){
						cout<<"Pin incorrecto"<<endl;

					}
					if(!confirm_access)
						id++;
				}

				if(confirm_access){
					cout<<"Usuario identifcado"<<endl;

					std::thread update_image(upload_image,&sck,&mydb,crop_face,file_name,id);
					update_image.detach();
					myleds.led_toggle(1,750);
					usleep(500*1000);
					myleds.led_toggle(1,750);
					usleep(500*1000);
					myleds.led_toggle(1,750);
				}
				else{
					num_int++;
					if (num_int<3){
						myleds.led_toggle(0,2*1000);
						continue;
					}
					else{
						num_int=0;
						cout<<"Numero de intentos superados"<<endl;
						remove(file_name.c_str());
						myleds.led_toggle(0,750);
						usleep(500*1000);
						myleds.led_toggle(0,750);
						usleep(500*1000);
						myleds.led_toggle(0,750);
						break;
					}
				}
				mydb.output.v2.clear();
				mydb.read_DB("users","id","NONE");
				std::vector <string> ids= mydb.output.v2;
				mydb.output.v2.clear();
				mydb.read_DB("users","n_im","NONE");
				std::vector <string> n_im= mydb.output.v2;
				mydb.output.v2.clear();
				int trainable=0;
				for (auto i:n_im){
					if (stoi(i)>=MIN_TRAIN_NIMG){
						trainable++;
					}
				}
				if(trainable>=2){
					cout<<"Modelo entrenable"<<endl;
					//sck.launch_training();
					std::thread start_train(thread_train, &sck);
					start_train.detach();
				}
				permiso(&mydb,&myleds,id,Mx);///////////////////
				break;
			}
		}

	}
	return 0;
	///
	}
	catch(...){
		myleds.set_led_value(0,1);
		return 0;
	}
	////
}

