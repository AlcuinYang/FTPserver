#include"server.h"
//主动休眠到达限速,参考网络代码
void sleep_opr_for_speed(struct timeval start,int send_len, double max_rate){
    struct timeval end;
    gettimeofday(&end,NULL);
    double cost_time = end.tv_sec - start.tv_sec + (double)(end.tv_usec - start.tv_usec)/1000000;
    double speed = send_len/(double)1024/cost_time;
    if(speed <= max_rate){ 
        return;
    }
    double sleep_time = (speed/max_rate-1)*cost_time;
    usleep(sleep_time*1000000);
}
//拿到权限号对应的字符串
void convert_str(int permit_num, char *permit_str)
{
    char tmp_buf[10];
    int r, w, x;
    for(int i = 6; i>=0; i-=3){
        //某个用户组对应的权限
        int permit_part_num = ((permit_num&ALLPERMS)>>i)&0x07;
        //通过位预算计算权限值
        r = (permit_part_num >> 2) & 0x1;
        w = (permit_part_num >> 1) & 0x1;
        x = (permit_part_num >> 0) & 0x1;
        memset(tmp_buf,0,10);
        if(r==1){
            strcat(tmp_buf,"r");
        }else{
            strcat(tmp_buf,"-");
        }
        if(w==1){
            strcat(tmp_buf,"w");
        }else{
            strcat(tmp_buf,"-");
        }
        if(x==1){
            strcat(tmp_buf,"x");
        }else{
            strcat(tmp_buf,"-");
        }
        strcat(permit_str,tmp_buf);
    }
}
//被动模式的前置操作
void prepose_opr_pasv(){
    //accept客户端
    printf("Passive Mode on\n");
    struct sockaddr_in addr;
    int len = sizeof(addr);
    socket_for_data = accept(socket_for_passive,(struct sockaddr*) &addr,&len);
    //取得ip地址和端口号
    dynamic_port = addr.sin_port;
    int ip_nums[4];
    int host_ip = (addr.sin_addr.s_addr);
    for(int i=0; i<4; i++){
        ip_nums[i] = (host_ip>>i*8)&0xff;
    }
    //IP地址
    sprintf(ip_addr_client,"%d.%d.%d.%d",ip_nums[0],ip_nums[1],ip_nums[2],ip_nums[3]);
    server_send_msg(socket_for_cmd,"150 accept client connect\n");
    printf("Connect client %s on TCP Port %d\n",ip_addr_client,dynamic_port);
}
//主动模式的前置操作
int prepose_opr_active(){
    printf("Active Mode on\n");
    //创建数据通道的客户端，在服务器这边
    socket_for_data = standard_socket(20);             
    //ftp client的地址
    struct sockaddr_in ftpclient_addr;
    //设置客户端地址
    memset(&ftpclient_addr, 0, sizeof(ftpclient_addr));
    ftpclient_addr.sin_family = AF_INET;
    ftpclient_addr.sin_port = htons(dynamic_port);
    ftpclient_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(connect(socket_for_data, (struct sockaddr *)&ftpclient_addr, sizeof(struct sockaddr_in)) < 0 )
    {
        server_send_msg(socket_for_cmd,"550 connect ftp client error\n");
        return -1;
    }
    printf("Connect client %s on TCP Port %d\n",ip_addr_client,dynamic_port);    
    server_send_msg(socket_for_cmd,"150 ftp server had connected client\n");
}
void tranfer_list(DIR *dir){
    //文件信息
    struct dirent *entry;
    struct stat statbuf;
    struct tm *time;
    char timebuff[SMALL_SIZE], current_dir[SMALL_SIZE];
    time_t rawtime;
    if(connect_mode == 0){//主动模式
        if(-1 == prepose_opr_active()){
            return;
        }
        while((entry=readdir(dir))!=NULL){
            stat(entry->d_name,&statbuf);
            rawtime = statbuf.st_mtime;
            time = localtime(&rawtime);
            strftime(timebuff,80,"%b %d %H:%M",time);
            //权限字符串
            char *permit_str = (char*)malloc(sizeof(char)*20);
            memset(permit_str,0,sizeof(char)*20);
            convert_str((statbuf.st_mode & ALLPERMS),permit_str);
            //直接一步写入到网络中
            char type = 'd';
            if(entry->d_type!=DT_DIR){
                type = '-';
            }
            dprintf(socket_for_data,"%c%s %5d %4d %4d %8d %s %s\r\n",
                    type,permit_str,statbuf.st_nlink,
                    statbuf.st_uid,statbuf.st_gid,
                    statbuf.st_size,timebuff,entry->d_name);
        }
        server_send_msg(socket_for_cmd,"226 data send success\n");
        close(socket_for_data);
    }else{//被动模式
        prepose_opr_pasv();
        while((entry=readdir(dir))!=NULL){
            stat(entry->d_name,&statbuf);
            rawtime = statbuf.st_mtime;
            time = localtime(&rawtime);
            strftime(timebuff,80,"%b %d %H:%M",time);
            //权限字符串
            char *permit_str = (char*)malloc(sizeof(char)*20);
            memset(permit_str,0,sizeof(char)*20);
            convert_str((statbuf.st_mode & ALLPERMS),permit_str);
            //直接一步写入到网络中
            char type = 'd';
            if(entry->d_type!=DT_DIR){
                type = '-';
            }
            dprintf(socket_for_data,"%c%s %5d %4d %4d %8d %s %s\r\n",
                    type,permit_str,statbuf.st_nlink,
                    statbuf.st_uid,statbuf.st_gid,
                    statbuf.st_size,timebuff,entry->d_name);
        }
        server_send_msg(socket_for_cmd,"226 data send success\n");
        close(socket_for_data);
        close(socket_for_passive);
    }
}
void  _sub(int fd){
    char databuf[BIG_SIZE];
    struct stat stat_buf;
    //得到文件size
    fstat(fd,&stat_buf);
    //统计时间
    struct timeval start;
    struct timeval end;
    double statis_cost;
    gettimeofday(&start,NULL);
    int recvLen;int sendLen;
    bzero(databuf,BIG_SIZE);
    while((recvLen = read(fd,databuf,BIG_SIZE)) >0){
        struct timeval during_start;
        gettimeofday(&during_start,NULL);
        sendLen= write(socket_for_data, databuf, recvLen);
        //开始限速
        if(limit_speed > 0){
            sleep_opr_for_speed(during_start,sendLen,limit_speed);
        }
        if(sendLen != recvLen){
            printf("test: send %d, read_len %d\n",sendLen,recvLen);
        }
        bzero(databuf,BIG_SIZE);
    }
    server_send_msg(socket_for_cmd,"226 finish download\n");
    gettimeofday(&end,NULL);
    statis_cost = end.tv_sec - start.tv_sec + (double)(end.tv_usec - start.tv_usec)/1000000;
    traffic_statis += stat_buf.st_size;
    double speed = stat_buf.st_size/(double)1024/statis_cost;
    printf("%d bytes sended in %0.2f secs (%0.1f KB/s)， total traffic=%d bytes\n",
        stat_buf.st_size, statis_cost, speed, traffic_statis);
}
void tranfer_retr(int fd){
    
    if(connect_mode == 0){
        if(-1 == prepose_opr_active()){
            return;
        }
        tranfer_retr_sub(fd);
        close(socket_for_data);
    }else{
        prepose_opr_pasv();
        tranfer_retr_sub(fd);
        close(socket_for_data);
        close(socket_for_passive);
    }
}
void tranfer_stor_sub(int fd){
    char databuf[BIG_SIZE];
    int recv_len;
    bzero(databuf, BIG_SIZE);
    int total_file_size =0;
    struct timeval start;
    struct timeval end;
    gettimeofday(&start,NULL);
    while((recv_len = read(socket_for_data, databuf, BIG_SIZE))>0){
        struct timeval during_start;
        gettimeofday(&during_start,NULL);
        int write_len =write(fd,databuf, recv_len);
        if(limit_speed>0){
            sleep_opr_for_speed(during_start,recv_len,limit_speed);
        }
        if(recv_len != write_len){
            printf("test: send %d, read_len %d\n",write_len,recv_len);
        }
        total_file_size += write_len;
        bzero(databuf, BIG_SIZE);
    }
    server_send_msg(socket_for_cmd,"226 upload file finish\n");
    gettimeofday(&end,NULL);
    double cost_time = end.tv_sec - start.tv_sec + (double)(end.tv_usec - start.tv_usec)/1000000;
    traffic_statis += total_file_size;
    float speed = total_file_size/(double)1024/cost_time;
    printf("%d bytes sended in %0.2f secs (%0.1f KB/s)， total traffic=%d bytes\n",
        total_file_size, cost_time, speed, traffic_statis);
}
void tranfer_stor(int fd){
    
    if(connect_mode == 0){
        if(-1 == prepose_opr_active()){
            return;
        }
        tranfer_stor_sub(fd);
        close(socket_for_data);
    }else{
        prepose_opr_pasv();
        tranfer_stor_sub(fd);
        close(socket_for_data);
        close(socket_for_passive);
    }
}

