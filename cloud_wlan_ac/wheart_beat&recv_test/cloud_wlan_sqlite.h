#ifndef CLOUD_WLAN_CFG_INIT_H_
#define CLOUD_WLAN_CFG_INIT_H_

#include <sqlite3.h>        //包含SQLite的头文件 

typedef struct sqlite3_res_t
{
	u32 nrow;
	u32 ncolumn;					  //行列数
	s8 ** sqlite3_data;
}sqlite3_res;

#define G_CW_SQL_BUF_LEN 512
extern s8 g_cw_sql[G_CW_SQL_BUF_LEN];  
extern sqlite3   *g_cw_db;  
extern s8	   *zErrMsg;  

extern u32 sqlite3_get_u32(sqlite3_res *res, u32 nrow, s8 *fields, u32 *dst);
extern u32 sqlite3_get_s32(sqlite3_res *res, u32 nrow, s8 *fields, s32 *dst);
extern u32 sqlite3_get_u8(sqlite3_res *res, u32 nrow, s8 *fields, u8 **dst);
extern u32 sqlite3_get_s8(sqlite3_res *res, u32 nrow, s8 *fields, s8 **dst);
extern u32 sqlite3_exec_get_res(sqlite3 *db, s8 *sql, sqlite3_res *res);
extern u32 sqlite3_exec_free_res(sqlite3_res *res);
extern u32 sqlite3_exec_unres(sqlite3 *db, s8 *sql);
extern u32 sqlite3_binary_write1(sqlite3 *db, s8 *sql, s8 *data, u32 data_len);
extern u32 sqlite3_binary_read(sqlite3 *db, s8 *sql, s8 **data, u32 column);

#endif

