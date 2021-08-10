#include"server.h"
//全局的登录信息
char *username[]={"yang"};
char *passwd[]={"yang"};
int permission[][4]={{1,1,1,1}};
double limit_speed = 1024;//单位kb
//定义变量
char client_name[SMALL_SIZE];
int login_state = -1; 1登录 0
int socket_for_cmd;
int socket_for_passive;
int dynamic_port;
int traffic_statis =0;
char ip_addr_client[SMALL_SIZE];
int client_permission[4]={0,0,0,0};
int connect_mode = 0;//默认主动模式，1为被动模式
int socket_for_data;
//用于缓存消息
char message[MID_SIZE];
char filename[MID_SIZE];
int main(){
    int socket_for_server = standard_socket(21);
    if(listen(socket_for_server,5) < 0){
        printf("listen error\n");
        return;
    }
    struct sockaddr_in sockaddr_in_client;
    int addr_size = sizeof(struct sockaddr_in);
    socket_for_cmd = accept(socket_for_server, (struct sockaddr*) &sockaddr_in_client,&addr_size);
    //如果有客户端连接上了，则开始发起请求
    server_send_msg(socket_for_cmd,"220 hi,I'm server\n");
    for(;;)
    {
        int recv_len;
        bzero(message,MID_SIZE);
        if((recv_len = read(socket_for_cmd, message, MID_SIZE))<=0){
            printf("server disconnect\n");
            return;
        }
        //展示接受到的消息
        printf("recv: %s\n", message);
        char command[SMALL_SIZE];bzero(command,SMALL_SIZE);
        char argument[SMALL_SIZE];bzero(argument,SMALL_SIZE);
        sscanf(message,"%s %s",command,argument);
        //命令相应操作
        if(strcmp("USER", command)==0){
            int size = sizeof(username)/sizeof(char*);
            for(int i=0;i<size;i++){
                if(strcmp(username[i],argument)==0){
                    login_state = 0;//用户名验证正确
                    strcpy(client_name, username[i]);
                    break;
                }
            }
            if(login_state == 0){
                
                server_send_msg(socket_for_cmd,"331 username valid\n");
            }else{
                server_send_msg(socket_for_cmd,"530 username invalid\n");
            }
        }
        else if(strcmp("PASS",command)==0){
            int size = sizeof(username)/sizeof(char*);
            for(int i=0;i<size;i++){
                if(strcmp(username[i],client_name)==0 &&strcmp(passwd[i],argument)==0){
                    login_state = 1;//用户名验证正确
                    client_permission[0] = permission[i][0];
                    client_permission[1] = permission[i][1];
                    client_permission[2] = permission[i][2];
                    client_permission[3] = permission[i][3];
                    break;
                }
            }
            if(login_state!=1){
                server_send_msg(socket_for_cmd,"500 password invalid\n");
            }else{//登录成功，show权限
                bzero(message,MID_SIZE);
                sprintf(message,"230 login success. you can do ");
                if(client_permission[0] ==1)
                {
                    strcat(message, "delete ");
                }
                if(client_permission[1] ==1)
                {
                    strcat(message, "cd ");
                }
                if(client_permission[2] ==1)
                {
                    strcat(message, "mkdir ");
                }
                if(client_permission[3] ==1)
                {
                    strcat(message, "rename ");
                }
                char tmp_buf[SMALL_SIZE];
                sprintf(tmp_buf,", limit speed is %0.1lf\n", limit_speed);
                strcat(message,tmp_buf);
                server_send_msg(socket_for_cmd,message);
            }
        }
        else if(strcmp("SYST",command)==0){
            server_send_msg(socket_for_cmd,"215 Linux\n");
        }
        else if(strcmp("PORT",command)==0){
            connect_mode = 0;
            int d_port[2], client_ip[4];
            sscanf(argument,"%d,%d,%d,%d,%d,%d",&client_ip[0],
            &client_ip[1],
            &client_ip[2],
            &client_ip[3],
            &d_port[0],
            &d_port[1]);
            //赋值ip
            sprintf(ip_addr_client,"%d.%d.%d.%d",client_ip[0],
            client_ip[1],
            client_ip[2],
            client_ip[3]);
            dynamic_port = 256 * d_port[0] + d_port[1];
            server_send_msg(socket_for_cmd,"200 dynamic port OK\n");
        }
        else if(strcmp("PASV",command)==0){
            connect_mode = 1;
            //随机端口号
            int d_port[2];
            srand(time(NULL));
            d_port[0] = 128 + (rand() % 64);
            d_port[1] = rand() % 0xff;
            socket_for_passive = standard_socket( d_port[0]*256 + d_port[1]);
            if(listen(socket_for_passive,5)<0){
                printf("listen pasv fail\n");
                break;
            }
            bzero(message,MID_SIZE);
            sprintf(message,"227 Passive Mode (127,0,0,1,%d,%d)\n",d_port[0],d_port[1]);
            server_send_msg(socket_for_cmd, message);
        }
        else if(strcmp("PWD",command)==0){
            printf("---begin %s run PWD\n",client_name);
            char cur_path[MID_SIZE];
            bzero(cur_path, MID_SIZE);
            bzero(message, MID_SIZE);
            //获取路径
            if(getcwd(cur_path,MID_SIZE)){
                sprintf(message,"257 path= %s\n", cur_path);
                server_send_msg(socket_for_cmd,message);
            }else{
                server_send_msg(socket_for_cmd,"550 exec pwd error\n");
            }
            printf("---end %s run PWD\n",client_name);
        }
        else if(strcmp("MKD",command)==0){
            printf("---begin %s run MKD\n",client_name);
            if(client_permission[2]==0){
                server_send_msg(socket_for_cmd,"500 please check you permission\n");
            }else{
                char cur_path[MID_SIZE];
                memset(cur_path,0,MID_SIZE);
                memset(message,0,MID_SIZE);
                //得到当前目录
                getcwd(cur_path,MID_SIZE);
                if(mkdir(argument,S_IRWXU)==0){
                    sprintf(message,"257 dir:%s/%s directory had created\n",cur_path,argument);
                    server_send_msg(socket_for_cmd,message);
                }else{
                    server_send_msg(socket_for_cmd,"550 create dir error\n");
                }
                
            }
            printf("---end %s run MKD\n",client_name);
        }
        else if(strcmp("CWD",command)==0){
            printf("---begin %s run CWD\n",client_name);
            if(client_permission[1]==0){
                server_send_msg(socket_for_cmd,"500 please check you permission\n");
            }else{
                chdir(argument);
                server_send_msg(socket_for_cmd,"250 cwd exec OK\n");
            }
            printf("---end %s run CWD\n",client_name);
        }
        else if(strcmp("DELE",command)==0){
            printf("---begin %s run DELE\n",client_name);
            if(client_permission[0] ==0){
                server_send_msg(socket_for_cmd,"500 please check you permission\n");
            }else{
                remove(argument);
                server_send_msg(socket_for_cmd,"250 dele exec OK\n");
            }
            printf
            ("---end %s run DELE\n",client_name);
        }
        else if(strcmp("LIST",command)==0){
            printf("---begin %s run LIST\n",client_name);
            //切换路径
            char path_old[MID_SIZE], path_new[MID_SIZE];
            memset(path_old,0,MID_SIZE);
            memset(path_new,0,MID_SIZE);
            getcwd(path_old,MID_SIZE);//拿到当前路径
            if(strlen(argument)>0 && argument[0]!='-'){
                chdir(argument);//需要切换路径
            }
            getcwd(path_new,MID_SIZE);//拿到新的目录
            DIR *dir;
            if((dir = opendir(path_new))==NULL){//打开目录
                server_send_msg(socket_for_cmd, "550 open dir error\n");
                continue;
            }
            tranfer_list(dir);//list功能
            //切换回原来路径
            chdir(path_old);
            closedir(dir);
            printf("---end %s run LIST\n",client_name);
        }
        else if(strcmp("RNFR",command)==0){
            printf("---begin %s run RNFR\n",client_name);
            if(client_permission[3] == 0){
                server_send_msg(socket_for_cmd,"500 please check you permission\n");
            }else{
                bzero(filename,MID_SIZE);
                strcpy(filename,argument);
                server_send_msg(socket_for_cmd,"350 rnfr OK\n");
            }
            printf("---end %s run RNFR\n",client_name);
        }
        else if(strcmp("RNTO",command)==0){
            printf("---begin %s run RNTO\n",client_name);
            if(client_permission[3] == 0){
                server_send_msg(socket_for_cmd,"500 please check you permission\n");
            }else{
                rename(filename,argument);
                server_send_msg(socket_for_cmd,"250 rnto OK\n");
            }
            printf("---end %s run RNTO\n",client_name);
        }
        else if(strcmp("RETR",command)==0){
            printf("---begin %s run RETR\n",client_name);
            int fd;
            //如果无法打开文件，则错误
            if(access(argument,R_OK)!=0 || (fd = open(argument,O_RDONLY)) < 0){
                server_send_msg(socket_for_cmd,"550 open file error\n");
                continue;
            }
            tranfer_retr(fd);
            close(fd);
            printf("---end %s run RETR\n",client_name);
        }
        else if(strcmp("STOR",command)==0){
            printf("---begin %s run STOR\n",client_name);
            int fd = open(argument,O_WRONLY|O_CREAT);
            if(fd < 0){
                server_send_msg(socket_for_cmd,"530 can't open file\n");
                continue;
            }
            tranfer_stor(fd);
            printf("---end %s run STOR\n",client_name);
        }
        else if(strcmp("QUIT",command)==0){
            printf("client disconnect\n");
            server_send_msg(socket_for_cmd,"221 you can go,I will wait for you\n");
            close(socket_for_cmd);
        }
        else{
            printf("invalid command\n");
            server_send_msg(socket_for_cmd,"500 invalid command\n");
        }
    }

}
//取得一个标准的socket套接字，内网ip，传入的port
int standard_socket(int port){
    int standard_st = socket(AF_INET, SOCK_STREAM, 0);
    //使得端口可以重用
    int reuse =1;
    setsockopt(standard_st, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
    struct sockaddr_in inner_addr;
    bzero(&inner_addr,sizeof(struct sockaddr_in));
    inner_addr.sin_port = htons(port);
    inner_addr.sin_family = AF_INET;
    inner_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(standard_st,(struct sockaddr*) &inner_addr, sizeof(struct sockaddr_in))<0){
        printf("bind socket error\n");
    }
    return standard_st;
}
void server_send_msg(int socket, char *msg){
    write(socket,msg,strlen(msg));
}