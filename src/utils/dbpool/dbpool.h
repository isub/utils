#ifndef _DBPOOL_H_
#define _DBPOOL_H_

#include <string>

#define OTL_ORA11G_R2
#define OTL_STL
#define OTL_ADD_NULL_TERMINATOR_TO_STRING_SIZE
#define OTL_STREAM_NO_PRIVATE_UNSIGNED_LONG_OPERATORS
#include "utils/otlv4.h"

#include "utils/log/log.h"

/* инициализация основных параметров */
int db_pool_init (CLog *p_pcoLog, std::string &p_strDBUsr, std::string &p_strDBPwd, std::string &p_strDBDscr, int p_iPoolSize);
/* деинициализация пула */
void db_pool_deinit ();

/* запрос указателя на объект класса для взаимодействия с БД */
otl_connect * db_pool_get ();

/* освобождение занятого объекта класса взаимодействия с БД */
int db_pool_release (otl_connect *p_pcoDBConn);

/* проверка работоспособности подключения к БД */
int db_pool_check (otl_connect &p_coDBConn);

/* переподключение к БД */
int db_pool_reconnect (otl_connect &p_coDBConn);

#endif /* _DBPOOL_H_ */
