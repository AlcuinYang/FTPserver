#ifndef SERVER_H
#define SERVER_H
#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include<string.h>

#define BIG_SIZE 2048//用于文件传输
#define MID_SIZE 1024//用于消息传输
#define SMALL_SIZE 128//用于存储信息
//客户端用户名
extern char client_name[SMALL_SIZE];
extern int login_state;//登录状态
extern int socket_for_cmd;//用于传输命令的套接字
extern int socket_for_data;
extern int socket_for_passive;//保存被动模式下，服务器的数据套接字
extern int dynamic_port;//用于保存主动模式和被动模式下，动态端口号
extern int traffic_statis;//服务器端传输流量
extern char ip_addr_client[SMALL_SIZE];//客户端ip
extern int client_permission[4];//客户端用户权限，针对dele,cwd,mkd,rename
extern int connect_mode;//连接模式
//限速
extern double limit_speed;
//用于rename时，from filename
extern char filename[MID_SIZE];
//函数
int standard_socket(int port);
void server_send_msg(int socket, char *msg);
//需要数据通道的命令函数
void tranfer_list(DIR *dir);
void tranfer_retr(int fd);
void tranfer_stor(int fd);
#endif
