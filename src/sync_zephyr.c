#include "sync.h"

#ifdef PS_SYNC_ZEPHYR

#include <zephyr/kernel.h>

int mutex_init(mutex_t *_m) {
	struct k_mutex **m = (struct k_mutex **) _m;
	*m = k_malloc(sizeof(struct k_mutex));
	return k_mutex_init(*m);
}

int mutex_lock(mutex_t _m) {
	struct k_mutex *m = (struct k_mutex *) _m;
	return k_mutex_lock(m, K_FOREVER);
}

int mutex_unlock(mutex_t _m) {
	struct k_mutex *m = (struct k_mutex *) _m;
	return k_mutex_unlock(m);
}

void mutex_destroy(mutex_t *_m) {
	(void) _m;
}

int semaphore_init(semaphore_t *_s, unsigned int value) {
	struct k_sem **s = (struct k_sem **) _s;
	*s = k_malloc(sizeof(struct k_sem));
	return k_sem_init(*s, value, value);
}

int semaphore_wait(semaphore_t _s, int32_t timeout_ms) {
	struct k_sem *s = (struct k_sem *) _s;
	if (timeout_ms < 0) {
		return k_sem_take(s, K_FOREVER);
	} else {
		return k_sem_take(s, K_MSEC(timeout_ms));
	}
}

int semaphore_post(semaphore_t _s) {
	struct k_sem *s = (struct k_sem *) _s;
	k_sem_give(s);
	return 0;
}

int semaphore_get(semaphore_t _s) {
	struct k_sem *s = (struct k_sem *) _s;
	return k_sem_count_get(s);
}

void semaphore_destroy(semaphore_t *_s) {
	(void) _s;
}

#endif
