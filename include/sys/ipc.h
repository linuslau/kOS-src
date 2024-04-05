
#ifndef	_KOS_IPC_H_
#define	_KOS_IPC_H_

/* Message mechanism is borrowed from MINIX */
#define	FD				u.m3.m3i1
#define	PATHNAME		u.m3.m3p1
#define	FLAGS			u.m3.m3i1
#define	NAME_LEN		u.m3.m3i2
#define	BUF_LEN			u.m3.m3i3
#define	CNT				u.m3.m3i2
#define	REQUEST			u.m3.m3i2
#define	PROC_NR			u.m3.m3i3
#define	NR_DEV_MINOR	u.m3.m3i4
#define	POSITION		u.m3.m3l1
#define	BUF				u.m3.m3p2
#define	OFFSET			u.m3.m3i2
#define	WHENCE			u.m3.m3i3
#define	PID				u.m3.m3i2
#define	RETVAL			u.m3.m3i1
#define	STATUS			u.m3.m3i1

struct mess1 {
	int 	m1i1;
	int 	m1i2;
	int 	m1i3;
	int 	m1i4;
};
struct mess2 {
	void* 	m2p1;
	void* 	m2p2;
	void* 	m2p3;
	void* 	m2p4;
};
struct mess3 {
	int		m3i1;
	int		m3i2;
	int		m3i3;
	int		m3i4;
	u64		m3l1;
	u64		m3l2;
	void*	m3p1;
	void*	m3p2;
};
typedef struct {
	int source;
	int type;
	union {
		struct mess1 m1;
		struct mess2 m2;
		struct mess3 m3;
	} u;
} message;

#endif /* _KOS_IPC_H_ */