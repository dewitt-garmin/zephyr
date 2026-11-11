/*
 * Copyright 2024 by Garmin Ltd. or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test priority set with mutexes held
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

/* Priority levels */
#define PRIO_HIGH K_PRIO_PREEMPT(1)
#define PRIO_MID  K_PRIO_PREEMPT(3)
#define PRIO_LOW  K_PRIO_PREEMPT(5)

#if Z_MUTEX_PI_ENABLED
static K_MUTEX_DEFINE(mutex);

static K_THREAD_STACK_DEFINE(stack_high, STACK_SIZE);

static struct k_thread t_high;

static void t_waiter(void *p1, void *p2, void *p3)
{
	struct k_mutex *m = (struct k_mutex *)p1;

	k_mutex_lock(m, K_FOREVER);
	k_mutex_unlock(m);
}
#endif /* Z_MUTEX_PI_ENABLED */

/**
 *
 * @brief
 *
 */

ZTEST(mutex_api, test_priority_set_with_mutex_locked)
{
#if Z_MUTEX_PI_ENABLED
	int orig_prio = k_thread_priority_get(k_current_get());

	k_thread_priority_set(k_current_get(), PRIO_LOW);

	k_mutex_init(&mutex);
	k_mutex_lock(&mutex, K_FOREVER);

	k_thread_create(&t_high, stack_high, STACK_SIZE, t_waiter, &mutex, NULL, NULL, PRIO_HIGH, 0,
			K_NO_WAIT);

	/* Wait for high priority thread to try to lock the mutex, boosting our priority */
	while (k_thread_priority_get(k_current_get()) != PRIO_HIGH) {
		k_sleep(K_MSEC(1));
	}

	k_thread_priority_set(k_current_get(), PRIO_MID);

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_HIGH,
		      "Expected priority %d, got %d", PRIO_HIGH,
		      k_thread_priority_get(k_current_get()));

	k_mutex_unlock(&mutex);

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_MID,
		      "Expected priority %d, got %d", PRIO_MID,
		      k_thread_priority_get(k_current_get()));

	k_thread_join(&t_high, K_FOREVER);

	k_thread_priority_set(k_current_get(), orig_prio);
#else
	ztest_test_skip();
#endif
}
