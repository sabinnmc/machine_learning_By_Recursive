/********************************************************************************
;*                                                                              *
;*        Copyright (C) 2021 PAL Inc. All Rights Reserved                       *
;*                                                                              *
;********************************************************************************
;*                                                                              *
;*        開発製品名    :    Visual Integration Unit                            *
;*                                                                              *
;*        ﾓｼﾞｭｰﾙ名      :    canthread.h                                        *
;*                                                                              *
;*        作　成　者    :    福屋　渡　                                         *
;*                                                                              *
;********************************************************************************/

#ifndef CANTHREAD_H_
#define CANTHREAD_H_

/*===    file include        ===*/
#include "Sysdef.h"

/*===    user definition        ===*/

/*===    external variable    ===*/

/*===    external function    ===*/

/*===    public function prototypes    ===*/

/*===    static function prototypes    ===*/

/*===    static typedef variable        ===*/
typedef struct {																	//
	T_CANFD_FRAME	buff[MAX_TXPACKET];
	uint			rp;
	uint			wp;
} TCanSendQueue;

typedef struct {																	//
	T_CANFD_FRAME	buff[MAX_RXPACKET];
	uint			rp;
	uint			wp;
} TCanRecvQueue;

typedef struct {																	//
	T_CANFD_FRAME	buff[MAX_RXPACKET];
	uint			rp;
	uint			wp;
} TCanBuffQueue;


/*===    static variable        ===*/

/*===    constant variable    ===*/

/*===    global variable        ===*/

/*===    implementation        ===*/

/***********************************************************************************************************************
Global functions
***********************************************************************************************************************/

#endif /* CANTHREAD_H_ */
