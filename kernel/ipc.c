
#include "shared.h"

static void block			(proc* p);
static void unblock			(proc* p);
static int  kernel_msg_send	(proc* current, int pid_receiver, message* m);
static int  kernel_msg_rcv	(proc* current, int pid_sender, message* m);
static int  check_deadlock	(int pid_sender, int pid_receiver);
static void assert_multiple	(proc* sender, proc* receiver, message *m, int assert_pos);

/* Ring 0 */
int sys_send_rcv_handler(int function, int pid_sender_receiver, message* m, proc* caller_proc)
{
	//intercept PRINTX as an exception
	if (m->type == PRINTX)
	{
		char * buf = m->BUF;/**< r/w buffer */
		sys_printx(function,pid_sender_receiver, buf, caller_proc);
		return 0;
	}
	assert(nested_int == 0);	/* make sure we are not in ring0 */
	assert((pid_sender_receiver >= 0 && pid_sender_receiver < NR_TASKS_NATIVE + NR_PROCS_MAX) ||
	       pid_sender_receiver == ANY ||
	       pid_sender_receiver == INTERRUPT);

	int ret = 0;
	int caller_pid = get_proc_pid(caller_proc);
	message* msg_la = (message*)proc_va2la(caller_pid, m);
	msg_la->source = caller_pid;

	assert(msg_la->source != pid_sender_receiver);

	if (function == SEND) {
		ret = kernel_msg_send(caller_proc, pid_sender_receiver, m);
		if (ret != 0)
			return ret;
	}
	else if (function == RECEIVE) {
		ret = kernel_msg_rcv(caller_proc, pid_sender_receiver, m);
		if (ret != 0)
			return ret;
	}
	else {
		panic("{sys_send_rcv_handler} invalid function: "
		      "%d (SEND:%d, RECEIVE:%d).", function, SEND, RECEIVE);
	}

	return 0;
}

/* Ring 0~3 */
void reset_msg(message* p)
{
	memset(p, 0, sizeof(message));
}

/* Ring 0, it is called after 'proc_state' is set (!= 0) */
static void block(proc* p)
{
	assert(p->proc_state);
	schedule();
}

/* Ring 0, a dummy routine. it is called, after `proc_state' is cleared (== 0). */
static void unblock(proc* p)
{
	assert(p->proc_state == RUNNABLE);
}

/* Ring 0, Check whether it is safe to send a message from pid_sender to pid_receiver.
 * The routine will detect if the messaging graph contains a cycle. For
 * instance, if we have procs trying to send messages like this:
 * A -> B -> C -> A, then a check_deadlock occurs, because all of them will
 * wait forever. If no cycles detected, it is considered as safe. */
static int check_deadlock(int pid_sender, int pid_receiver)
{
	proc* p = proc_table + pid_receiver;
	while (1) {
		if (p->proc_state & SENDING) {
			if (p->pid_sendingto == pid_sender) {
				/* print the chain */
				p = proc_table + pid_receiver;
				printl("=_=%s", p->name);
				do {
					assert(p->pending_msg);
					p = proc_table + p->pid_sendingto;
					printl("->%s", p->name);
				} while (p != proc_table + pid_sender);
				printl("=_=");

				return 1;
			}
			p = proc_table + p->pid_sendingto;
		}
		else {
			break;
		}
	}
	return 0;
}

/* Ring 0 */
static int kernel_msg_send(proc* current, int pid_receiver, message* m)
{
	proc* sender 	= current;
	proc* receiver 	= proc_table + pid_receiver; /* proc pid_receiver */

	assert(get_proc_pid(sender) != pid_receiver);

	/* check for check_deadlock here */
	if (check_deadlock(get_proc_pid(sender), pid_receiver)) {
		panic(">>check_deadlock<< %s->%s", sender->name, receiver->name);
	}

	if ((receiver->proc_state & RECEIVING) &&
	    (receiver->pid_recvingfrom == get_proc_pid(sender) || receiver->pid_recvingfrom == ANY)) {
		assert_multiple(0, receiver, m, 1);

		phys_copy(proc_va2la(pid_receiver, receiver->pending_msg), proc_va2la(get_proc_pid(sender), m), sizeof(message));

		receiver->pending_msg 		= 0;
		receiver->proc_state 		&= ~RECEIVING; /* pid_receiver has received the msg */
		receiver->pid_recvingfrom 	= NO_TASK;
		unblock(receiver);

		assert_multiple(sender, receiver, 0, 2);
	}
	else { /* pid_receiver is not waiting for the msg */
		sender->proc_state |= SENDING;
		assert(sender->proc_state == SENDING);
		sender->pid_sendingto = pid_receiver;
		sender->pending_msg = m;

		/* append to the sending queue */
		proc * p;
		if (receiver->q_pending_proc) {
			p = receiver->q_pending_proc;
			while (p->q_pending_proc)
				p = p->q_pending_proc;
			p->q_pending_proc = sender;
		}
		else {
			receiver->q_pending_proc = sender;
		}
		sender->q_pending_proc = 0;

		block(sender);

		assert_multiple(sender, 0, 0, 3);
	}

	return 0;
}

/* Ring 0 */
static int kernel_msg_rcv(proc* current, int pid_sender, message* m)
{
	proc* receiver = current; 
	proc* sender = 0;
	proc* prev = 0;
	int cp_msg_from_sender = 0;

	assert(get_proc_pid(receiver) != pid_sender);

	if ((receiver->has_int_msg) &&
	    ((pid_sender == ANY) || (pid_sender == INTERRUPT))) {
		/* There is an interrupt needs p_who_wanna_recv's handling and
		 * p_who_wanna_recv is ready to handle it.
		 */
		message msg;
		reset_msg(&msg);
		msg.source = INTERRUPT;
		msg.type = HW_INT;
		assert(m);
		phys_copy(proc_va2la(get_proc_pid(receiver), m), &msg,
			  sizeof(message));

		receiver->has_int_msg = 0;

		assert_multiple(0, receiver, 0, 4);
		return 0;
	}

	if (pid_sender == ANY) {
		/* receiver is ready to receive messages from
		 * ANY proc, we'll check the sending queue and pick the first proc in it. */
		if (receiver->q_pending_proc) {
			sender = receiver->q_pending_proc;
			cp_msg_from_sender = 1;
			assert_multiple(sender, receiver, 0, 5);
		}
	}
	else {
		/* receiver wants to receive a message from a certain proc: src. */
		sender = &proc_table[pid_sender];

		if ((sender->proc_state & SENDING) &&
		    (sender->pid_sendingto == get_proc_pid(receiver))) {
			/* Perfect, pid_sender is sending a message to
			 * receiver.
			 */
			cp_msg_from_sender = 1;

			proc* p = receiver->q_pending_proc;
			assert(p); /* sender must have been appended to the
				    * queue, so the queue must not be NULL
				    */
			while (p) {
				assert(sender->proc_state & SENDING);
				if (get_proc_pid(p) == pid_sender) { /* if p is the one */
					sender = p;
					break;
				}
				prev = p;
				p = p->q_pending_proc;
			}
			assert_multiple(sender, receiver, 0, 6);
		}
	}

	if (cp_msg_from_sender) {
		/* It's determined from which proc the message will
		 * be copied. Note that this proc must have been
		 * waiting for this moment in the queue, so we should
		 * remove it from the queue.
		 */
		if (sender == receiver->q_pending_proc) { /* the 1st one */
			assert(prev == 0);
			receiver->q_pending_proc = sender->q_pending_proc;
			sender->q_pending_proc = 0;
		}
		else {
			assert(prev);
			prev->q_pending_proc = sender->q_pending_proc;
			sender->q_pending_proc = 0;
		}

		assert_multiple(sender, 0, m, 7);
		/* copy the message */
		phys_copy(proc_va2la(get_proc_pid(receiver), m),
				  proc_va2la(get_proc_pid(sender), sender->pending_msg),
				  sizeof(message));

		sender->pending_msg		= 	0;
		sender->pid_sendingto 	= 	NO_TASK;
		sender->proc_state		&= 	~SENDING;
		unblock(sender);
	}
	else {  /* nobody's sending any msg */
		/* Set proc_state so that receiver will not be scheduled until it is unblocked. */
		receiver->proc_state |= RECEIVING;
		receiver->pending_msg = m;

		if (pid_sender == ANY)
			receiver->pid_recvingfrom = ANY;
		else
			receiver->pid_recvingfrom = get_proc_pid(sender);

		block(receiver);
		assert_multiple(0, receiver, 0, 8);
	}

	return 0;
}

/* Ring 0, Inform a proc that an interrupt has occured. */
void hw_int_notification(int task_nr)
{
	proc* p = proc_table + task_nr;

	if ((p->proc_state & RECEIVING) && /* pid_receiver is waiting for the msg */
	    ((p->pid_recvingfrom == INTERRUPT) || (p->pid_recvingfrom == ANY))) {
		p->pending_msg->source = INTERRUPT;
		p->pending_msg->type = HW_INT;
		p->pending_msg = 0;
		p->has_int_msg = 0;
		p->proc_state &= ~RECEIVING; /* pid_receiver has received the msg */
		p->pid_recvingfrom = NO_TASK;
		assert(p->proc_state == 0);
		unblock(p);

		assert_multiple(0, p, 0, 9);
	}
	else {
		p->has_int_msg = 1;
	}
}

void assert_multiple(proc* sender, proc* receiver, message *m, int assert_pos)
{
	switch (assert_pos) {
		case 1:
			assert(receiver->pending_msg);
			assert(m);
			break;

		case 2:
			assert(receiver->proc_state == 0);
			assert(receiver->pending_msg == 0);
			assert(receiver->pid_recvingfrom == NO_TASK);
			assert(receiver->pid_sendingto == NO_TASK);
			assert(sender->proc_state == 0);
			assert(sender->pending_msg == 0);
			assert(sender->pid_recvingfrom == NO_TASK);
			assert(sender->pid_sendingto == NO_TASK);
			break;

		case 3:
			assert(sender->proc_state == SENDING);
			assert(sender->pending_msg != 0);
			assert(sender->pid_recvingfrom == NO_TASK);
			//assert(sender->pid_sendingto == pid_receiver);
			break;

		case 4:
			assert(receiver->proc_state == 0);
			assert(receiver->pending_msg == 0);
			assert(receiver->pid_sendingto == NO_TASK);
			assert(receiver->has_int_msg == 0);
			break;

		case 5:
			assert(receiver->proc_state == 0);
			assert(receiver->pending_msg == 0);
			assert(receiver->pid_recvingfrom == NO_TASK);
			assert(receiver->pid_sendingto == NO_TASK);
			assert(receiver->q_pending_proc != 0);
			assert(sender->proc_state == SENDING);
			assert(sender->pending_msg != 0);
			assert(sender->pid_recvingfrom == NO_TASK);
			assert(sender->pid_sendingto == get_proc_pid(receiver));
			break;

		case 6:
			assert(receiver->proc_state == 0);
			assert(receiver->pending_msg == 0);
			assert(receiver->pid_recvingfrom == NO_TASK);
			assert(receiver->pid_sendingto == NO_TASK);
			assert(receiver->q_pending_proc != 0);
			assert(sender->proc_state == SENDING);
			assert(sender->pending_msg != 0);
			assert(sender->pid_recvingfrom == NO_TASK);
			assert(sender->pid_sendingto == get_proc_pid(receiver));
			break;

		case 7:
			assert(m);
			assert(sender->pending_msg);
			break;

		case 8:
			assert(receiver->proc_state == RECEIVING);
			assert(receiver->pending_msg != 0);
			assert(receiver->pid_recvingfrom != NO_TASK);
			assert(receiver->pid_sendingto == NO_TASK);
			assert(receiver->has_int_msg == 0);
			break;

		case 9:
			assert(receiver->proc_state == 0);
			assert(receiver->pending_msg == 0);
			assert(receiver->pid_recvingfrom == NO_TASK);
			assert(receiver->pid_sendingto == NO_TASK);
			break;

		default:
			break;
	}
}