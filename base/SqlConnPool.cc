#include "SqlConnPool.h"


MYSQL* SqlConnPool::GetConnection() {
	{
		MutexGuard lock(mutex_);
		if (sqlConnList_.empty()) return nullptr;
	}
	
	MYSQL *sqlConn = nullptr;
	reserveSem_.wait();
	
	{
		MutexGuard lock(mutex_);
		sqlConn = sqlConnList_.front();
		sqlConnList_.pop_front();
		--freeSqlConn_;
		++curSqlConn_;
	}

	return sqlConn;
}
	
bool SqlConnPool::ReleaseConnection(MYSQL* sqlConn) {
	if (sqlConn == nullptr) return false;
	{
		MutexGuard lock(mutex_);

		sqlConnList_.push_back(sqlConn);
		++freeSqlConn_;
		--curSqlConn_;
	}
	reserveSem_.post();
	return true;
}

int SqlConnPool::GetFreeConn() const {
	return freeSqlConn_;
}

void SqlConnPool::DestroyPool() {
	MutexGuard lock(mutex_);
	for (auto &it : sqlConnList_) {
		mysql_close(it);
	}
	sqlConnList_.clear();
	curSqlConn_ = freeSqlConn_ = 0;
}


SqlConnPool* SqlConnPool::GetInstance() {
	static SqlConnPool sqlConnPool;
	return &sqlConnPool;
}


void SqlConnPool::init(std::string url, std::string User, std::string PassWord, std::string DataBaseName, int Port, int MaxConn, int closeLog) {
	url_ = url;
	user_ = User;
	passWord_ = PassWord;
	dataBaseName_ = DataBaseName;
	port_ = Port;
	closeLog_ = closeLog;

	for (int i = 0; i < MaxConn; ++i) {
		MYSQL *sqlConn = nullptr;
		sqlConn = mysql_init(sqlConn);
		if (sqlConn == nullptr) {
			LOG_ERROR("MYSQL ERROR");
			exit(1);
		}
		sqlConn = mysql_real_connect(sqlConn, url.c_str(), User.c_str(), PassWord.c_str(), DataBaseName.c_str(), Port, nullptr, 0);

		if (sqlConn == nullptr) {
			LOG_ERROR("MYSQL ERROR");
			exit(1);
		}
		sqlConnList_.push_back(sqlConn);
	}
	
	reserveSem_ = Semaphore(MaxConn);
	freeSqlConn_ = maxSqlConn_ = MaxConn;
}
