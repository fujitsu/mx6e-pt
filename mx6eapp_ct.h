/******************************************************************************/
/* ファイル名 : mx6eapp_ct.h                                                  */
/* 機能概要   : 単体試験 ヘッダファイル                                       */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/

#ifndef __MX6EAPP_CT_H__
#   define __MX6EAPP_CT_H__
#   if defined(CT)
#      include "mx6eapp.h"
void                            ct(mx6e_handler_t * handler);
#   endif
#endif
