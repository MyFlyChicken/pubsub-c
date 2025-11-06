#ifdef PS_SYNC_RTTHREAD

#include <stdlib.h>
#include <rtthread.h>

int mutex_init(mutex_t *_m) {
	rt_mutex_t **m = (rt_mutex_t **) _m;
	*m = rt_malloc(sizeof(struct rt_mutex));
	return rt_mutex_init(*m, "ps_mutex", RT_IPC_FLAG_FIFO);
}

int mutex_lock(mutex_t _m) {
	rt_mutex_t *m = (rt_mutex_t *) _m;
	return rt_mutex_take(m, RT_WAITING_FOREVER);
}

int mutex_unlock(mutex_t _m) {
	rt_mutex_t *m = (rt_mutex_t *) _m;
	return rt_mutex_release(m);
}

int mutex_destroy(mutex_t *_m) {
	rt_mutex_t **m = (rt_mutex_t **) _m;
	int res = rt_mutex_detach(*m);
	rt_free(*m);
	*m = NULL;
	return res;
}

int semaphore_init(semaphore_t *_s, unsigned int value) {
	rt_sem_t **s = (rt_sem_t **) _s;
	*s = rt_malloc(sizeof(struct rt_semaphore));
	return rt_sem_init(*s, "ps_sema", value, RT_IPC_FLAG_FIFO);
}

int semaphore_wait(semaphore_t s, int32_t timeout_ms) {
	rt_sem_t *sem = (rt_sem_t *) s;
	if (timeout_ms < 0) {
		return rt_sem_take(sem, RT_WAITING_FOREVER);
	} else {
		return rt_sem_take(sem, rt_tick_from_millisecond(timeout_ms));
	}
}

int semaphore_post(semaphore_t s) {
	rt_sem_t *sem = (rt_sem_t *) s;
	return rt_sem_release(sem);
}

int semaphore_get(semaphore_t s) {
	rt_sem_t *sem = (rt_sem_t *) s;
	return sem->value;
}

void semaphore_destroy(semaphore_t *_s) {
	rt_sem_t **s = (rt_sem_t **) _s;
	rt_sem_detach(*s);
	rt_free(*s);
	*s = NULL;
}

#endif
