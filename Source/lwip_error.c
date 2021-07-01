/*
 * lwip_error.c
 *
 *  Created on: 10 май 2020 г.
 *      Author: admin
 */

#include <stdio.h>
#include <string.h>

#include "lwip/err.h"

#include "lwip_error.h"

//*************************************************************************************************
// Локальные константы
//*************************************************************************************************
static const char msg_err_ok[]      = "No error, everything OK.";
static const char msg_err_mem[]     = "Out of memory error.";
static const char msg_err_buf[]     = "Buffer error.";
static const char msg_err_timeout[] = "Timeout.";
static const char msg_err_rte[]     = "Routing problem.";
static const char msg_err_inprog[]  = "Operation in progress.";
static const char msg_err_val[]     = "Illegal value.";
static const char msg_err_block[]   = "Operation would block.";
static const char msg_err_use[]     = "Address in use.";
static const char msg_err_ready[]   = "Already connecting.";
static const char msg_err_isconn[]  = "Conn already established.";
static const char msg_err_conn[]    = "Not connected.";
static const char msg_err_if[]      = "Low-level netif error.";
static const char msg_err_abort[]   = "Connection aborted.";
static const char msg_err_rst[]     = "Connection reset.";
static const char msg_err_clsd[]    = "Connection closed.";
static const char msg_err_arg[]     = "Illegal argument.";

//*************************************************************************************************
// Локальные переменные
//*************************************************************************************************
static char undef[10];

//*************************************************************************************************
// Возвращает текстовую расшифровку кода ошибки LWIP
// err_t code - код ошибки.
// return     - текстовое сообщение.
//*************************************************************************************************
char *GetLWIPErrorMsg( err_t code ) {

    if ( code == ERR_OK )
        return (char *)msg_err_ok;
    if ( code == ERR_MEM )
        return (char *)msg_err_mem;
    if ( code == ERR_BUF )
        return (char *)msg_err_buf;
    if ( code == ERR_TIMEOUT )
        return (char *)msg_err_timeout;
    if ( code == ERR_RTE )
        return (char *)msg_err_rte;
    if ( code == ERR_INPROGRESS )
        return (char *)msg_err_inprog;
    if ( code == ERR_VAL )
        return (char *)msg_err_val;
    if ( code == ERR_WOULDBLOCK )
        return (char *)msg_err_block;
    if ( code == ERR_USE )
        return (char *)msg_err_use;
    if ( code == ERR_ALREADY )
        return (char *)msg_err_ready;
    if ( code == ERR_ISCONN )
        return (char *)msg_err_isconn;
    if ( code == ERR_CONN )
        return (char *)msg_err_conn;
    if ( code == ERR_IF )
        return (char *)msg_err_if;
    if ( code == ERR_ABRT )
        return (char *)msg_err_abort;
    if ( code == ERR_RST )
        return (char *)msg_err_rst;
    if ( code == ERR_CLSD )
        return (char *)msg_err_clsd;
    if ( code == ERR_ARG )
        return (char *)msg_err_arg;
    sprintf( undef, "%d", code );
    return undef;
}
