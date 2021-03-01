#ifndef BASE_SQLCONNPOOL_H
#define BASE_SQLCONNPOOL_H


#include "Log.h"
#include <mysql/mysql.h>
#include <list>
#include "Mutex.h"
#include "Semaphore.h"

class SqlConnPool {
public:
	SqlConnPool() {}
	~SqlConnPool() {DestroyPool();}
	
	MYSQL* GetConnection();
	bool ReleaseConnection(MYSQL* sqlConn);
	int GetFreeConn() const;
	void DestroyPool();

	static SqlConnPool* GetInstance();

	void init(std::string url, std::string User, std::string PassWord, std::string DataBaseName, int Port, int MaxConn, int closeLog); 
private:

	std::string url_;
	std::string user_;
	std::string passWord_;
	std::string dataBaseName_;
	int port_;
	int closeLog_;

	int maxSqlConn_;
	int curSqlConn_ = 0;
	int freeSqlConn_;

	Semaphore reserveSem_; 
	Mutex mutex_;

	std::list<MYSQL*> sqlConnList_;
};

#endif
