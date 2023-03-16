#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "chatroom_server.h"
#include <sys/epoll.h>
#include <sys/types.h>
#include <pthread.h>
/**function: to init User_Condition
 * input:
 *           int online:        user's condition;    0 means off,    1 means online      104/204/304 means differen error
 *           string in_host:    user's IP 
 *           int in_port        user's port listen to connection
 *           int in_user_id     user's ID
 * */
User_Condition::User_Condition(int in_online,std::string in_host="",int in_port=0,int in_user_id=0){
    user_id=in_user_id;
    online=in_online;
    host=in_host;
    port=in_port;
}


Database::Database(){
    if(mysql_init(&mysql_base)==nullptr){
#ifdef DEBUG
        std::cout<<"mysql_base init error:"<<mysql_error(&mysql_base);
#endif
        exit(-1);
    }
}

Database::~Database(){
    mysql_close(&mysql_base);
}

/**function: connect to a appionted Database
 * input:
 *           string host:      mysql server's IP
 *           string root:      user's name of Database
 *           string password   user's password
 * */
void Database::connect_database(std::string host="localhost",std::string user="root",std::string password="zns2002014"){
        if(mysql_real_connect(&mysql_base,host.c_str(),user.c_str(),password.c_str(),"chatroom",0,nullptr,0)==nullptr){
#ifdef DEBUG
            std::cout<<"connect database error:"<<mysql_error(&mysql_base);
#endif
        }
}

/**function: query appionted user's password
 * input:
 *           int user_id:      user's ID
 * output:
 *           string            password of appionted user
 * */
std::string Database::query_password(int user_id){
    std::string number_str=std::to_string(user_id);
    std::string query_str="select * from password where user_id="+number_str;
    if(mysql_real_query(&mysql_base,query_str.c_str(),query_str.length())){
        return std::string();
    }
    MYSQL_RES* query_result=mysql_store_result(&mysql_base);
    if(query_result==nullptr){
        return std::string();
    }
    MYSQL_ROW row=mysql_fetch_row(query_result);
    if(row==nullptr){
        return std::string();
    }
    return row[1];
}

/**function: query appionted user's condition,include online/off  IP   port 
 *input:
 *           int user_id;
 *output:
 *           struct User_Condition
 * */
User_Condition Database::query_condition(int user_id){
    std::string user_id_str=std::to_string(user_id);
    std::string query_str="select * from user where user_id="+user_id_str;
    if(mysql_real_query(&mysql_base,query_str.c_str(),query_str.length())){
        return User_Condition(104);
    }
    MYSQL_RES* query_result=mysql_store_result(&mysql_base);
    if(query_result==nullptr){
        return User_Condition(204);
    }
    MYSQL_ROW row=mysql_fetch_row(query_result);
    if(row==nullptr){
        return User_Condition(304);
    }
    return User_Condition(std::atoi(row[3]),row[1],std::atoi(row[2]),std::atoi(row[0]));
}

/**function: add data in Database chatroom;     
 *           insert user's ID and password in table password;  
 *           insert user's ID,user's port,user's IP,online/off in table user;
 *input:
 *           string password:user's password sign int
 *output:
 *           int    user's ID
 * */
int Database::user_sign_in(std::string password){
    std::string query_add="insert into password values(null,'"+password+"')";
    if(mysql_real_query(&mysql_base,query_add.c_str(),query_add.length())){
        return -1;
    }
    std::string query_insert_id="select last_insert_id()";
    if(mysql_real_query(&mysql_base,query_insert_id.c_str(),query_insert_id.length())){
        return -1;
    }
    MYSQL_RES* insert_id_res=mysql_store_result(&mysql_base);
    if(insert_id_res==nullptr){
        return -1;
    }
    MYSQL_ROW insert_id_row=mysql_fetch_row(insert_id_res);
    if(insert_id_row==nullptr){
        return -1;
    }
    std::string user_id=insert_id_row[0];
    std::string query_insert_user="insert into user values("+user_id+",' ',0,0)";
    if(mysql_real_query(&mysql_base,query_insert_user.c_str(),query_insert_user.length())){
        return -1;
    }
    return std::atoi(user_id.c_str());
}

/**function: update data in user where user's ID=user_id
 *input:
 *           int user_id
 *           int host:     IP
 *           int port
 *           int online
 *output:
 *           bool          
 * */
bool Database::user_update(int user_id,std::string host,int port,int online){
    std::string query_update_user="update user set host='"+host+"',port="+std::to_string(port)+",online="+std::to_string(online)+" where user_id="+std::to_string(user_id);
    if(mysql_real_query(&mysql_base,query_update_user.c_str(),query_update_user.length())){
        return false;
    }
    return true;
}
 
#ifdef EPOLL_POOL
//init static lock
pthread_mutex_t Server::socketfd_array_lock=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Server::database_lock=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Server::wait_up=PTHREAD_COND_INITIALIZER;
Server::Server():Database(){
     //init epollfd and socketfd
     epollfd=epoll_create(MAXEPOLL);
     socketfd=socket(AF_INET,SOCK_STREAM,0);
     //init address_server and address_client
     bzero(&address_server,sizeof(address_server));
     bzero(&address_client,sizeof(address_client));
     address_server.sin_family=AF_INET;
     address_server.sin_addr.s_addr=inet_addr("192.168.80.207");
     address_server.sin_port=htons(40000);
     //bind socketfd with server port
     if(bind(socketfd,(struct sockaddr*)&address_server,sizeof(sockaddr_in))){
#ifdef DEBUG  
         std::cout<<"blind error.\n";
#endif
            exit(-1);
     }
     //socketfd listen connection
     if(listen(socketfd,10)==-1){
#ifdef DEBUG
            std::cout<<"listen error.\n";
#endif
            exit(-1);
     }
     std::cout<<"listen succeed.\n";
     //struct epoll_event lis_event[MAXEPOLL];
     event_fd=new struct epoll_event[MAXEPOLL];
     struct epoll_event event;
     event.events=EPOLLIN;
     event.data.fd=socketfd;
     int epoll_add=epoll_ctl(epollfd,EPOLL_CTL_ADD,socketfd,&event);
     if(epoll_add==-1){
#ifdef DEBUG
         std::cout<<"epoll_ctl error.\n";
#endif
         exit(-1);
     }
}

Server::~Server(){
    delete[] event_fd;
}
//function is used for client requestion
void* thread_work(void* arg){
    while(1){
        Server *work=(Server*)arg;
        char buf[MAXBUF];
#ifdef DEBUG
        std::cout<<"pthread is created.\n";
#endif
        //waitong for client's data.Access shared memory,need locked
        pthread_mutex_lock(&Server::socketfd_array_lock);
        pthread_cond_wait(&Server::wait_up,&Server::socketfd_array_lock);
        int fd=work->socketfd_array.front();
        work->socketfd_array.pop();
        pthread_mutex_unlock(&Server::socketfd_array_lock);
        int ret=recv(fd,buf,sizeof(buf),0);
        if(ret==-1){
#ifdef DEBUG
            std::cout<<"recive error.\n";
#endif
            epoll_ctl(work->epollfd,EPOLL_CTL_DEL,fd,NULL);
            continue;
        }
        //Access database,need locked. 
        pthread_mutex_lock(&Server::database_lock);
        //query other user's condition
        if(buf[0]=='q'){
            struct User_Condition ask_for=User_Condition(0);
            memcpy((void*)&ask_for,(void*)&buf[1],sizeof(User_Condition));
            ask_for=work->query_condition(ask_for.user_id);
            pthread_mutex_unlock(&Server::database_lock);
            buf[0]='q';
            memcpy((void*)&buf[1],(void*)&ask_for,sizeof(ask_for));
            ret=send(fd,&buf,sizeof(buf),0);
            if(ret==-1){
#ifdef DEBUG
                std::cout<<"send error.\n";
#endif
                continue;
            }
        }
        //user reuquest for sing-up
        else if(buf[0]=='s'){
            Sign_apply sign;
            memcpy((void*)&sign,(void*)&buf[1],sizeof(sign));
            int user_id=work->user_sign_in(sign.password);
            pthread_mutex_unlock(&Server::database_lock);
            buf[0]='u';
            memcpy((void*)&buf[1],(void*)&user_id,sizeof(user_id));
            int ret=send(fd,&buf,sizeof(buf),0);
            if(ret==-1){
#ifdef DEBUG
                std::cout<<"send error.\n";
#endif
                continue;
            }
        }
        //asking for password when login
        else if(buf[0]=='p'){
            struct User_Condition ask_for=User_Condition(0);
            memcpy((void*)&ask_for,(void*)&buf[1],sizeof(ask_for));
            std::string password=work->query_password(ask_for.user_id);
            pthread_mutex_unlock(&Server::database_lock);
            buf[0]='p';
            memcpy((void*)&buf[1],(void*)&password,sizeof(password));
            int ret=send(fd,&buf,sizeof(buf),0);
            if(ret==-1){
#ifdef DEBUG
                std::cout<<"send error.\n";
#endif
                continue;
            }
        } 
    }
}
//mian thread and socket is block
void Server::run(){
    int pthread_ret[MAXEPOLL]={0};
    for(int i=0;i<MAXEPOLL;i++){
        pthread_ret[i]=pthread_create(&work_thread[i],NULL,thread_work,(void*)this);
        if(pthread_ret[i]){
#ifdef DEBUG
            std::cout<<"pthread "<<i<<"create error.\n";
#endif
            exit(-1);
        }
    }
#ifdef DEBUG
    std::cout<<"waiting for connection.\n";
    std::cout<<"succeed create thread.\n";
#endif
    while(1){
        int event_wait=epoll_wait(epollfd,event_fd,MAXEPOLL,-1);
        if(event_wait==-1){
#ifdef DEBUG
            std::cout<<"epoll wait error.\n";
#endif
            exit(-1);
       }
#ifdef DEBUG
        std::cout<<"connect quest.\n";
#endif
        for(int i=0;i<event_wait;i++){
            if(event_fd[i].data.fd==socketfd){
                int new_fd=accept(socketfd,(struct sockaddr*)&address_client,(socklen_t*)sizeof(address_client));
                if(new_fd==-1){
#ifdef DEBUG
                    std::cout<<"accpet error.\n";
#endif
                    exit(-1);
                }
                epoll_ctl(epollfd,EPOLL_CTL_ADD,new_fd,NULL);
            }
            else{
                pthread_cond_signal(&wait_up);
            }
        }
    }
}
#endif

int main(){
   Server server;
   server.run();
   return 0; 
}
