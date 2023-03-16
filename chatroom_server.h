#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <queue>

#define MAXBUF 1024
#define MAXEPOLL 5
#define MAXLISTEN 20
#define MYHOST "localhost"
#define MYPORT 40000
#define EPOLL_POOL
#define DEBUG
//used for client query other's postion
struct User_Condition{
    int user_id;
    int online;
    std::string host;
    int port;
    User_Condition(int in_online,std::string host,int port,int in_user_id);
};

//used for client user sign
struct Sign_apply{
    std::string password;
    std::string host;
    int port;
};

enum Type{
    Signin=0,        //sign in 
    query            //query for other postion
};

class Database{

private:
    MYSQL mysql_base;

public:
    Database();
    ~Database();
    void connect_database(std::string host,std::string user,std::string password);
    std::string query_password(int user_id);
    User_Condition query_condition(int user_id);
    int user_sign_in(std::string passwor);
    bool user_update(int user_id,std::string host,int port,int online);
};

#ifdef EPOLL_POOL
class Server:public Database
{
public:
    //char buf[MAXBUF];
    int socketfd;
    struct sockaddr_in address_server,address_client;
    struct epoll_event *event_fd;
    std::queue<int> socketfd_array;
    int epollfd;
    static pthread_mutex_t socketfd_array_lock,database_lock;
    static pthread_cond_t wait_up;
    pthread_t work_thread[MAXEPOLL];
public:
    Server();
    ~Server();
    void run();
};
    void* thread_work(void* arg);
#endif
