#ifndef BASE_SQLCONNRAII_H
#define BASE_SQLCONNRAII_H

#include "SqlConnPool.h"

class SqlConnRAII {
public:
	SqlConnRAII(MYSQL** SQL, SqlConnPool* sqlConnPool) : sqlConnPoolRAII_(sqlConnPool) {
		sqlConnRAII_ = *SQL = sqlConnPool->GetConnection();
	}
	~SqlConnRAII() {
		sqlConnPoolRAII_->ReleaseConnection(sqlConnRAII_);
	}
private:
	MYSQL* sqlConnRAII_;
	SqlConnPool* sqlConnPoolRAII_;
};

#endif
