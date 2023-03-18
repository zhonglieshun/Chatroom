# 1、架构
## 1.1 定义类：
服务器端分别定义了数据库类（class Database)和客户端请求处理类（class Server)；Database作为基类，由Server公有继承。
## 1.2 类功能：
Database封装了MySql数据库处理函数，为Server提供数据库接口。
Server建立TCP监听接口、使用epoll实现了I/O多路复用、创建了固定数量的子线程处理客户端请求、使用主线程处理用户连接并通过条件变量调度子线程。
## 1.3 Database类成员函数：
void connect_database(string host,int user,string password).......**连接指定数据库**  
sring query_password(int user_id).......**查询用户登陆密码**  
User_condition query_condition(int user_id).......**查询用户登陆状态、IP、端口**  
int user_sign(string password).......**用户注册**  
bool user_update(int user_id,string host,int port,int online).......**更新用户状态**
## 1.4 Server成员函数：
Server().......**建立监听端口、初始化成员变量 ** 
void run().......**处理用户连接、线程调度**
## 1.5 子线程函数：
void* thread(void* arg).......**逻辑函数，处理用户请求，通过pthread_cond_wait()阻塞等待调用**
# 2、问题
1. Server的封装性：由于规划时想将所有处理用户请求的功能封装在Server类中，将子线程主函数作为了Server的成员函数。所以不得已将该函数改为普通函数并将Server类的所有成员变量改为
public，将Database改为public继承。　　
2. 错误检测：由于时间关系，未使用标准的报错函数和接口。　　
3. 应用层传输协议：本阶段仅简单实现用户请求的分类，未设计通信协议。　　
4. 线程数：采用固定数量的线程，未现实线程的动态创建
# 3、规划
1. 客户端创建：解决NAT穿透问题。　　
2. 传输协议：规范服务端-客户端、客户端-客户端之间的报文格式。　　
3. 线程池的实现：根据线程使用情况动态规划线程数量　　　
4. 类封装性：重新规化函数功能。　　
5. 差错检测：规范函数报错和返回。　　
6. 数据库访问使用Redis和MySql组合使用降低数据查询时间。　　
7. 异步处理网卡数据使用异步I/O处理数据拷贝
## ......
