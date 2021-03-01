#include "config.h"

int main(int argc, char *argv[]) {
    //需要修改的数据库信息,登录名,密码,库名
    string user = "root";
    string passwd = "123qweASD";
    string databasename = "yourdb";
	
    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    //初始化
    server.init(config.PORT, user, passwd, databasename, config.LOGWrite, 
                config.OPT_LINGER, config.TRIGMode,  config.sql_num,  config.thread_num, 
                config.close_log, config.actor_model);
    

    //日志
    server.initLog();

    //数据库
    server.initSqlConnPool();

    //线程池
    server.initThreadPool();

    //触发模式
    server.initTrigerMode();

    //监听
    server.eventListen();

    //运行
    server.eventLoop();

    return 0;
}
