#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "pubsub.h"

/* Helper functions */
static void check_leak(void) {
	ps_clean_sticky("");
	PUBSUB_ASSERT(ps_stats_live_msg() == 0);
	PUBSUB_ASSERT(ps_stats_live_subscribers() == 0);
}

static void *inc_thread(void *v) {
	(void) v; // unused
	ps_subscriber_t *s = ps_new_subscriber(10, PS_STRLIST("fun.inc"));
	PS_PUB_BOOL_FL("thread.ready", true, PS_FL_STICKY);
	ps_msg_t *msg = ps_get(s, 5000);
	PUBSUB_ASSERT(msg != NULL);
	PUBSUB_ASSERT(PS_IS_INT(msg));
	PS_PUB_INT(msg->rtopic, msg->int_val + 1); // Response
	ps_unref_msg(msg);
	ps_free_subscriber(s);
	return NULL;
}

static int new_msg_cb_touch;
static ps_subscriber_t *new_msg_cb_subscriber;

static void new_msg_cb(ps_subscriber_t *su) {
	new_msg_cb_touch++;
	new_msg_cb_subscriber = su;
}

static int non_empty_cb_touch;
static ps_subscriber_t *non_empty_cb_subscriber;

static void non_empty_cb(ps_subscriber_t *su) {
	non_empty_cb_touch++;
	non_empty_cb_subscriber = su;
}

/* End helper functions*/

/* Test Functions */
void test_subscriptions(void) {
	printf("Test subscriptions\n");
	ps_subscriber_t *s1 = ps_new_subscriber(10, NULL);
	PUBSUB_ASSERT(ps_subscribe(s1, "foo.bar") == 0);
	PUBSUB_ASSERT(ps_subscribe(s1, "foo.bar") == -1);
	PUBSUB_ASSERT(ps_unsubscribe(s1, "foo.baz") == -1);
	ps_subscriber_t *s2 = ps_new_subscriber(10, PS_STRLIST("foo", "baz"));
	PUBSUB_ASSERT(ps_unsubscribe(s1, "foo") == -1);
	PUBSUB_ASSERT(ps_num_subs(s1) == 1);
	PUBSUB_ASSERT(ps_num_subs(s2) == 2);
	ps_unsubscribe(s2, "baz");
	PUBSUB_ASSERT(ps_num_subs(s1) == 1);
	PUBSUB_ASSERT(ps_num_subs(s2) == 1);
	ps_free_subscriber(s1);
	ps_free_subscriber(s2);
	check_leak();
}

void test_hidden_subscription(void) {
	printf("Test hidden subscription\n");
	ps_subscriber_t *s1 = ps_new_subscriber(10, PS_STRLIST("foo.bar"));
	ps_subscriber_t *s2 = ps_new_subscriber(10, PS_STRLIST("foo.bar" PS_SUB_HIDDEN));
	ps_subscriber_t *s3 = ps_new_subscriber(10, NULL);
	ps_subscribe_flags(s3, "foo.bar", &(ps_sub_flags_t){.hidden = true});
	PUBSUB_ASSERT(PS_PUB_NIL("foo.bar") == 1);
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	PUBSUB_ASSERT(ps_waiting(s2) == 1);
	PUBSUB_ASSERT(ps_waiting(s3) == 1);
	ps_free_subscriber(s1);
	ps_free_subscriber(s2);
	ps_free_subscriber(s3);
	check_leak();
}

void test_weird_subscription(void) {
	printf("Test weird subscription\n");
	ps_subscriber_t *su = ps_new_subscriber(10, NULL);

	PUBSUB_ASSERT(ps_subscribe(su, "") == 0); // Test global suscription
	PUBSUB_ASSERT(PS_PUB_NIL("foo") == 1);
	PUBSUB_ASSERT(PS_PUB_NIL_FL("foo", PS_FL_NONRECURSIVE) == 0);
	PUBSUB_ASSERT(ps_waiting(su) == 1);
	ps_flush(su);
	ps_subscribe(su, "bar 123!#"); // Test undefined flags
	PS_PUB_NIL("bar");
	PUBSUB_ASSERT(ps_waiting(su) == 2);
	ps_flush(su);
	ps_unsubscribe_all(su);

	ps_subscribe(su, "baz p"); // Test malformed priority flag
	PS_PUB_NIL("baz");
	PUBSUB_ASSERT(ps_waiting(su) == 1);
	ps_flush(su);

	ps_subscribe(su, "baz pkk"); // Test priority flag with nondigit parameters
	PS_PUB_NIL("baz");
	PUBSUB_ASSERT(ps_waiting(su) == 1);
	ps_flush(su);

	ps_free_subscriber(su);
	check_leak();
}

void test_subs_count(void) {
	printf("Test subs_count\n");
	PUBSUB_ASSERT(ps_subs_count(NULL) == 0);
	PUBSUB_ASSERT(ps_subs_count("") == 0);
	PUBSUB_ASSERT(ps_subs_count("foo") == 0);
	PUBSUB_ASSERT(ps_subs_count("foo.bar") == 0);
	PUBSUB_ASSERT(ps_subs_count("bar") == 0);
	PUBSUB_ASSERT(ps_subs_count("baz") == 0);
	ps_subscriber_t *s1 = ps_new_subscriber(10, PS_STRLIST("foo.bar"));
	ps_subscriber_t *s2 = ps_new_subscriber(10, PS_STRLIST("foo", "baz"));
	PUBSUB_ASSERT(ps_subs_count("foo") == 1);
	PUBSUB_ASSERT(ps_subs_count("foo" PS_SUB_PRIO(5)) == 1);
	PUBSUB_ASSERT(ps_subs_count("foo.bar") == 2);
	PUBSUB_ASSERT(ps_subs_count("bar") == 0);
	PUBSUB_ASSERT(ps_subs_count("baz") == 1);
	ps_free_subscriber(s1);
	ps_free_subscriber(s2);
	check_leak();
}

void test_subscribe_many(void) {
	printf("Test subscribe/unsubscribe many\n");
	ps_subscriber_t *s1 = ps_new_subscriber(10, NULL);
	PUBSUB_ASSERT(ps_subscribe_many(s1, PS_STRLIST("foo", "bar", "baz")) == 3);
	PUBSUB_ASSERT(ps_num_subs(s1) == 3);
	PUBSUB_ASSERT(ps_unsubscribe_many(s1, PS_STRLIST("foo", "bar", "baz")) == 3);
	PUBSUB_ASSERT(ps_num_subs(s1) == 0);
	ps_free_subscriber(s1);
	check_leak();
}

void test_publish(void) {
	printf("Test publish\n");
	ps_subscriber_t *s1 = ps_new_subscriber(10, PS_STRLIST("foo.bar"));
	ps_subscriber_t *s2 = ps_new_subscriber(10, PS_STRLIST("foo", "baz"));
	PS_PUB_BOOL("foo.bar", true);
	PS_PUB_BOOL("foo", true);
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	PUBSUB_ASSERT(ps_waiting(s2) == 2);
	PUBSUB_ASSERT(ps_stats_live_msg() == 2);
	ps_flush(s1);
	PUBSUB_ASSERT(ps_stats_live_msg() == 2);
	ps_flush(s2);
	PUBSUB_ASSERT(ps_stats_live_msg() == 0);
	PUBSUB_ASSERT(ps_waiting(s1) == 0);
	PUBSUB_ASSERT(ps_waiting(s2) == 0);
	ps_free_subscriber(s1);
	ps_free_subscriber(s2);
	check_leak();
}

void test_sticky(void) {
	printf("Test sticky\n");
	PS_PUB_INT_FL("foo", 1, PS_FL_STICKY);
	PS_PUB_INT_FL("foo", 2, PS_FL_STICKY); // Last sticky override previous
	ps_subscriber_t *s1 = ps_new_subscriber(10, PS_STRLIST("foo"));
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	ps_msg_t *msg = ps_get(s1, -1);
	PUBSUB_ASSERT(msg->int_val == 2);
	ps_unref_msg(msg);
	ps_free_subscriber(s1);
	PUBSUB_ASSERT(ps_stats_live_msg() == 1); // The sticky message
	PS_PUB_INT("foo", 3); // Publish normal message on the same path, unsticks previous sitcked message
	PUBSUB_ASSERT(ps_stats_live_msg() == 0);
	check_leak();
}

void test_clean_sticky(void) {
	printf("Test clean sticky\n");
	PS_PUB_INT_FL("foo.bar.baz", 1, PS_FL_STICKY);
	PS_PUB_INT_FL("foo.fiz.fuz", 1, PS_FL_STICKY);
	PUBSUB_ASSERT(ps_stats_live_msg() == 2); // The sticky messages;
	ps_clean_sticky("foo.bar");              // Remove sticky messages from "foo.bar" prefix
	PUBSUB_ASSERT(ps_stats_live_msg() == 1); // The sticky messages;
	ps_clean_sticky("foo");                  // Remove sticky messages from "foo" prefix
	PUBSUB_ASSERT(ps_stats_live_msg() == 0); // The sticky messages;
	check_leak();
}

void test_clean_all_children_sticky(void) {
	printf("Test clean all children sticky\n");
	PS_PUB_INT_FL("foo.bar.baz", 1, PS_FL_STICKY);
	PS_PUB_INT_FL("foo.fiz.fuz", 1, PS_FL_STICKY);
	PUBSUB_ASSERT(ps_stats_live_msg() == 2); // The sticky messages;
	ps_clean_sticky("foo");                  // Remove sticky messages from "foo" prefix
	PUBSUB_ASSERT(ps_stats_live_msg() == 0); // The sticky messages;
	check_leak();
}

void test_no_sticky_flag(void) {
	printf("Test no sticky flag\n");
	PS_PUB_INT_FL("foo", 1, PS_FL_STICKY);
	PS_PUB_INT_FL("bar", 1, PS_FL_STICKY);
	ps_subscriber_t *s1 =
	ps_new_subscriber(10, PS_STRLIST("foo" PS_SUB_NOSTICKY)); // Ignore sticky mesages published before subscription
	ps_subscribe_flags(s1, "bar", &(ps_sub_flags_t){.no_sticky = true});
	PUBSUB_ASSERT(ps_waiting(s1) == 0);
	PS_PUB_INT_FL("foo", 2, PS_FL_STICKY); // This is a new message (published after subscription)
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	ps_free_subscriber(s1);
	check_leak();
}

void test_child_sticky_flag(void) {
	printf("Test child sticky flag\n");
	PS_PUB_NIL_FL("foo.bar.baz", PS_FL_STICKY);
	PS_PUB_NIL_FL("foo.bar", PS_FL_STICKY);
	PS_PUB_NIL_FL("foo", PS_FL_STICKY);

	ps_subscriber_t *s1 = ps_new_subscriber(10, PS_STRLIST("foo" PS_SUB_CHILDSTICKY)); // Get all child sticky messages
	PUBSUB_ASSERT(ps_waiting(s1) == 3);
	ps_free_subscriber(s1);

	s1 = ps_new_subscriber(10, NULL); // Get all child sticky messages
	ps_subscribe_flags(s1, "foo.bar", &(ps_sub_flags_t){.child_sticky = true});
	PUBSUB_ASSERT(ps_waiting(s1) == 2);
	ps_free_subscriber(s1);

	s1 = ps_new_subscriber(10, PS_STRLIST("foo.bar.baz" PS_SUB_CHILDSTICKY)); // Get all child sticky messages
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	ps_free_subscriber(s1);

	check_leak();
}

void test_no_recursive(void) {
	printf("Test no recursive\n");
	ps_subscriber_t *s1 = ps_new_subscriber(10, PS_STRLIST("foo.bar"));
	ps_subscriber_t *s2 = ps_new_subscriber(10, PS_STRLIST("foo"));
	PS_PUB_INT_FL("foo.bar", 1, PS_FL_NONRECURSIVE);
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	PUBSUB_ASSERT(ps_waiting(s2) == 0);
	ps_free_subscriber(s1);
	ps_free_subscriber(s2);
	check_leak();
}

void test_on_empty(void) {
	ps_msg_t *msg;
	printf("Test on empty\n");
	ps_subscriber_t *s1 = ps_new_subscriber(10, PS_STRLIST("foo" PS_SUB_EMPTY));
	ps_subscriber_t *s2 = ps_new_subscriber(10, NULL);
	ps_subscribe_flags(s2, "foo", &(ps_sub_flags_t){.on_empty = true});
	PS_PUB_NIL("foo.bar");
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	PUBSUB_ASSERT(ps_waiting(s2) == 1);
	PS_PUB_NIL("foo.bar");
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	PUBSUB_ASSERT(ps_waiting(s2) == 1);
	msg = ps_get(s1, 10);
	PUBSUB_ASSERT(PS_IS_NIL(msg));
	ps_unref_msg(msg);
	PUBSUB_ASSERT(ps_waiting(s1) == 0);
	PS_PUB_NIL("foo.bar");
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	PS_PUB_NIL("foo.bar");
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	ps_free_subscriber(s1);
	ps_free_subscriber(s2);
	check_leak();
}

void test_unsub_on_empty(void) {
	printf("Test unsub on empty\n");
	ps_subscriber_t *s1 = ps_new_subscriber(10, PS_STRLIST("foo" PS_SUB_EMPTY, "bar" PS_SUB_EMPTY));
	ps_unsubscribe(s1, "foo");
	PS_PUB_NIL("foo.bar");
	PUBSUB_ASSERT(ps_waiting(s1) == 0);
	ps_unsubscribe(s1, "bar" PS_SUB_EMPTY);
	PS_PUB_NIL("bar");
	PUBSUB_ASSERT(ps_waiting(s1) == 0);
	ps_free_subscriber(s1);
	check_leak();
}

void test_pub_get(void) {
	printf("Test pub->get\n");
	ps_msg_t *msg;
	ps_subscriber_t *s1 = ps_new_subscriber(10, PS_STRLIST("foo.bar"));
	PS_PUB_INT("foo.bar", 1);
	PS_PUB_DBL("foo.bar", 1.25);
	PS_PUB_STR("foo.bar", "Hello");
	PS_PUB_ERR("foo.bar", -1, "Bad result");
	PS_PUB_BUF("foo.bar", PUBSUB_MALLOC(10), 10, PUBSUB_FREE);
	PS_PUB_PTR("foo.bar", s1);

	msg = ps_get(s1, 10);
	PUBSUB_ASSERT(PS_IS_INT(msg) && msg->int_val == 1);
	ps_unref_msg(msg);
	msg = ps_get(s1, 10);
	PUBSUB_ASSERT(PS_IS_DBL(msg) && msg->dbl_val == 1.25);
	ps_unref_msg(msg);
	msg = ps_get(s1, 10);
	PUBSUB_ASSERT(PS_IS_STR(msg) && strcmp(msg->str_val, "Hello") == 0);
	ps_unref_msg(msg);
	msg = ps_get(s1, 10);
	PUBSUB_ASSERT(PS_IS_ERR(msg) && (msg->err_val.id == -1) && (strcmp(msg->err_val.desc, "Bad result") == 0));
	ps_unref_msg(msg);
	msg = ps_get(s1, 10);
	PUBSUB_ASSERT(PS_IS_BUF(msg) && (msg->buf_val.sz == 10));
	ps_unref_msg(msg);
	msg = ps_get(s1, 10);
	PUBSUB_ASSERT(PS_IS_PTR(msg) && (msg->ptr_val == s1));
	ps_unref_msg(msg);

	msg = ps_get(s1, 1);
	PUBSUB_ASSERT(msg == NULL);

	PUBSUB_ASSERT(ps_waiting(s1) == 0);
	ps_free_subscriber(s1);
	check_leak();
}

void test_overflow(void) {
	printf("Test overflow\n");
	ps_msg_t *msg;
	ps_subscriber_t *s1 = ps_new_subscriber(2, PS_STRLIST("foo.bar"));
	PS_PUB_INT("foo.bar", 1);
	PS_PUB_INT("foo.bar", 2);
	PS_PUB_INT("foo.bar", 3);
	PUBSUB_ASSERT(ps_overflow(s1) == 1);
	PUBSUB_ASSERT(ps_overflow(s1) == 0);
	msg = ps_get(s1, 10);
	PUBSUB_ASSERT(PS_IS_INT(msg) && msg->int_val == 1);
	ps_unref_msg(msg);
	msg = ps_get(s1, 10);
	PUBSUB_ASSERT(PS_IS_INT(msg) && msg->int_val == 2);
	ps_unref_msg(msg);

	PUBSUB_ASSERT(ps_waiting(s1) == 0);
	ps_free_subscriber(s1);
	check_leak();
}

void test_new_msg_cb(void) {
	printf("Test new msg callback\n");

	new_msg_cb_touch = 0;
	new_msg_cb_subscriber = NULL;
	non_empty_cb_touch = 0;
	non_empty_cb_subscriber = NULL;

	ps_subscriber_t *s1 = ps_new_subscriber(10, PS_STRLIST("foo.bar"));
	PS_PUB_INT("foo.bar", 1);
	ps_set_new_msg_cb(s1, new_msg_cb);
	PUBSUB_ASSERT(new_msg_cb_touch == 1);
	PUBSUB_ASSERT(new_msg_cb_subscriber == s1);
	PS_PUB_INT("foo.bar", 1);
	ps_set_non_empty_cb(s1, non_empty_cb);
	PUBSUB_ASSERT(non_empty_cb_touch == 1);
	PUBSUB_ASSERT(non_empty_cb_subscriber == s1);
	PS_PUB_INT("foo.bar", 1);
	PUBSUB_ASSERT(non_empty_cb_touch == 1);
	PUBSUB_ASSERT(new_msg_cb_touch == 3);
	PUBSUB_ASSERT(ps_waiting(s1) == 3);
	ps_flush(s1);
	PS_PUB_INT("foo.bar", 1);
	PUBSUB_ASSERT(new_msg_cb_touch == 4);
	PUBSUB_ASSERT(non_empty_cb_touch == 2);
	ps_free_subscriber(s1);
	check_leak();
}

void test_call(void) {
	printf("Test call\n");
	ps_msg_t *msg = NULL;
	pthread_t thread;
	pthread_create(&thread, NULL, inc_thread, NULL);
	msg = ps_wait_one("thread.ready", 5000);
	PUBSUB_ASSERT(msg != NULL && msg->bool_val == true);
	ps_unref_msg(msg);
	msg = PS_CALL_INT("fun.inc", 25, 1000);
	PUBSUB_ASSERT(msg != NULL && msg->int_val == 26);
	ps_unref_msg(msg);

	msg = PS_CALL_INT("fun.other", 0, 1000000); // If message is not delivered timeout is triggered
	PUBSUB_ASSERT(msg == NULL);
	ps_unref_msg(msg); // unref_msg suppots NULL messages

	pthread_join(thread, NULL);
	check_leak();
}

void test_no_return_path(void) {
	printf("Test no return path\n");
	ps_msg_t *msg = NULL;
	pthread_t thread;
	pthread_create(&thread, NULL, inc_thread, NULL);
	msg = ps_wait_one("thread.ready", 5000);
	PUBSUB_ASSERT(msg != NULL && msg->bool_val == true);
	ps_unref_msg(msg);
	PS_PUB_INT("fun.inc", 25);
	pthread_join(thread, NULL);
	check_leak();
}

void test_topic_prefix_suffix(void) {
	printf("Test has_topic, has_topic_prefix, has_topic_suffix\n");
	ps_msg_t *msg = NULL;
	PUBSUB_ASSERT(!ps_has_topic(msg, "foo.bar"));
	PUBSUB_ASSERT(!ps_has_topic_prefix(msg, "foo.bar"));
	PUBSUB_ASSERT(!ps_has_topic_suffix(msg, "foo.bar"));

	ps_subscriber_t *s1 = ps_new_subscriber(2, PS_STRLIST("foo.bar"));
	PS_PUB_NIL("foo.bar");
	msg = ps_get(s1, 10);
	PUBSUB_ASSERT(ps_has_topic(msg, "foo.bar"));
	PUBSUB_ASSERT(ps_has_topic(msg, "foo.bar" PS_SUB_PRIO(5)));
	PUBSUB_ASSERT(ps_has_topic_prefix(msg, "foo.bar"));
	PUBSUB_ASSERT(ps_has_topic_prefix(msg, "foo.bar" PS_SUB_PRIO(5)));
	PUBSUB_ASSERT(ps_has_topic_prefix(msg, "foo."));
	PUBSUB_ASSERT(ps_has_topic_suffix(msg, ".bar"));

	PUBSUB_ASSERT(!ps_has_topic(msg, "foo.baz"));
	PUBSUB_ASSERT(!ps_has_topic_prefix(msg, "baz."));
	PUBSUB_ASSERT(!ps_has_topic_suffix(msg, ".baz"));
	PUBSUB_ASSERT(!ps_has_topic_suffix(msg, "this.is.a.very.large.topic"));

	ps_msg_set_topic(msg, "foo.old");
	ps_msg_set_topic(msg, "foo.new");
	PUBSUB_ASSERT(ps_has_topic(msg, "foo.new"));

	ps_msg_set_rtopic(msg, "foo.old");
	ps_msg_set_rtopic(msg, "foo.new");
	PUBSUB_ASSERT(strcmp(msg->rtopic, "foo.new") == 0);

	ps_unref_msg(msg);
	ps_free_subscriber(s1);
	check_leak();
}

void test_msg_getset(void) {
	printf("Test msg getset values\n");
	ps_subscriber_t *su = ps_new_subscriber(1, PS_STRLIST("foo"));
	ps_msg_t *msg = NULL;

	PS_PUB_INT("foo", 42);
	msg = ps_get(su, 1000);
	PUBSUB_ASSERT(ps_msg_value_int(msg) == 42);
	PUBSUB_ASSERT(ps_msg_value_double(msg) == 42);
	PUBSUB_ASSERT(ps_msg_value_bool(msg));
	ps_unref_msg(msg);

	PS_PUB_DBL("foo", 123);
	msg = ps_get(su, 1000);
	PUBSUB_ASSERT(ps_msg_value_int(msg) == 123);
	PUBSUB_ASSERT(ps_msg_value_double(msg) == 123);
	PUBSUB_ASSERT(ps_msg_value_bool(msg));
	ps_unref_msg(msg);

	PS_PUB_BOOL("foo", true);
	msg = ps_get(su, 1000);
	PUBSUB_ASSERT(ps_msg_value_int(msg) == 1);
	PUBSUB_ASSERT(ps_msg_value_double(msg) == 1);
	PUBSUB_ASSERT(ps_msg_value_bool(msg));
	ps_unref_msg(msg);

	PS_PUB_NIL("foo");
	msg = ps_get(su, 1000);
	PUBSUB_ASSERT(ps_msg_value_int(msg) == 0);
	PUBSUB_ASSERT(ps_msg_value_double(msg) == 0);
	PUBSUB_ASSERT(!ps_msg_value_bool(msg));

	ps_msg_set_value_int(msg, 987);
	PUBSUB_ASSERT(ps_msg_value_int(msg) == 987);
	ps_unref_msg(msg);
	msg = NULL;
	ps_free_subscriber(su);
	check_leak();
}

void test_dup_msg(void) {
	printf("Test dup msg\n");
	ps_msg_t *msg = NULL;
	ps_msg_t *dup = NULL;

	char str[] = "bar";
	msg = ps_new_msg("foo", PS_STR_TYP, str);
	ps_msg_set_rtopic(msg, "baz");
	int live_msg = ps_stats_live_msg();
	dup = ps_dup_msg(msg);
	PUBSUB_ASSERT(ps_stats_live_msg() == live_msg + 1);
	PUBSUB_ASSERT(msg->_ref == 1);
	PUBSUB_ASSERT(dup->_ref == 1);
	PUBSUB_ASSERT(strcmp(msg->topic, dup->topic) == 0);
	PUBSUB_ASSERT(strcmp(msg->rtopic, dup->rtopic) == 0);
	PUBSUB_ASSERT(msg->flags == dup->flags);
	PUBSUB_ASSERT(dup->str_val != msg->str_val);
	PUBSUB_ASSERT(strcmp(msg->str_val, "bar") == 0);
	ps_unref_msg(msg);
	ps_unref_msg(dup);

	uint8_t *buf = PUBSUB_CALLOC(3, sizeof(uint8_t));
	buf[0] = 0x42;
	msg = ps_new_msg("foo", PS_BUF_TYP, (void *) buf, 3, PUBSUB_FREE);
	ps_msg_set_rtopic(msg, "baz");
	dup = ps_dup_msg(msg);
	PUBSUB_ASSERT(dup->buf_val.ptr != msg->buf_val.ptr);
	PUBSUB_ASSERT(((uint8_t *) dup->buf_val.ptr)[0] == 0x42);
	PUBSUB_ASSERT(dup->buf_val.sz == 3);
	PUBSUB_ASSERT(dup->buf_val.dtor == PUBSUB_FREE);
	ps_unref_msg(msg);
	ps_unref_msg(dup);

	char err[] = "error";
	msg = ps_new_msg("foo", PS_ERR_TYP, 42, err);
	ps_msg_set_rtopic(msg, "baz");
	dup = ps_dup_msg(msg);
	PUBSUB_ASSERT(dup->err_val.desc != msg->err_val.desc);
	PUBSUB_ASSERT(strcmp(dup->err_val.desc, msg->err_val.desc) == 0);
	PUBSUB_ASSERT(dup->err_val.id == 42);
	ps_unref_msg(msg);
	ps_unref_msg(dup);
	check_leak();
}

void test_subscriber_userdata(void) {
	printf("Test userdata\n");
	int foo = 42;
	ps_subscriber_t *su = ps_new_subscriber(1, PS_STRLIST("foo"));
	ps_subscriber_user_data_set(su, &foo);
	PUBSUB_ASSERT(ps_subscriber_user_data(su) == &foo);
	ps_free_subscriber(su);
	check_leak();
}

void test_priority(void) {
	printf("Test priority\n");

#ifdef PS_QUEUE_LL
	printf(">> WARNING: The selected queue implemetation doesn't support priorities\n");
	return;
#endif

	ps_subscriber_t *su = ps_new_subscriber(3, PS_STRLIST("lost", "foo", "bar" PS_SUB_PRIO(1)));
	ps_subscribe_flags(su, "baz", &(ps_sub_flags_t){.priority = 9});
	PS_PUB_NIL("foo" PS_SUB_PRIO(1)); // priorities on publishes should be ignored
	PS_PUB_NIL("lost");
	PS_PUB_NIL("baz");
	PS_PUB_NIL("bar");

	PUBSUB_ASSERT(ps_overflow(su) == 1);

	ps_msg_t *msg = NULL;
	msg = ps_get(su, 1000);
	PUBSUB_ASSERT(ps_has_topic(msg, "baz"));
	ps_unref_msg(msg);

	msg = ps_get(su, 1000);
	PUBSUB_ASSERT(ps_has_topic(msg, "bar"));
	ps_unref_msg(msg);

	msg = ps_get(su, 1000);
	PUBSUB_ASSERT(ps_has_topic(msg, "foo"));
	ps_unref_msg(msg);

	PUBSUB_ASSERT(ps_waiting(su) == 0);

	PS_PUB_NIL("foo");
	PS_PUB_NIL("baz");
	PS_PUB_NIL("bar");

	ps_free_subscriber(su);
	check_leak();
}

void test_compatibility(void) {
#ifndef PS_DEPRECATE_NO_PREFIX
	printf("Test compatibility\n");
	ps_subscriber_t *s1 = ps_new_subscriber(10, STRLIST("foo.bar"));
	ps_subscriber_t *s2 = ps_new_subscriber(10, STRLIST("foo", "baz"));
	PUB_INT("foo.bar", true);
	PUB_BOOL("foo", true);
	PUBSUB_ASSERT(ps_waiting(s1) == 1);
	PUBSUB_ASSERT(ps_waiting(s2) == 2);

	new_msg_cb_touch = 0;
	new_msg_cb_subscriber = NULL;
	ps_set_new_msg_cb(s1, new_msg_cb);
	PUBSUB_ASSERT(new_msg_cb_touch == 1);
	PUBSUB_ASSERT(new_msg_cb_subscriber == s1);

	ps_free_subscriber(s1);
	ps_free_subscriber(s2);
	check_leak();
#endif
}

void run_all(void) {
	test_subscriptions();
	test_hidden_subscription();
	test_weird_subscription();
	test_subscribe_many();
	test_subs_count();
	test_publish();
	test_sticky();
	test_clean_sticky();
	test_clean_all_children_sticky();
	test_no_sticky_flag();
	test_child_sticky_flag();
	test_no_recursive();
	test_on_empty();
	test_unsub_on_empty();
	test_pub_get();
	test_overflow();
	test_new_msg_cb();
	test_call();
	test_no_return_path();
	test_topic_prefix_suffix();
	test_msg_getset();
	test_dup_msg();
	test_subscriber_userdata();
	test_priority();
	test_compatibility();
	printf("All tests passed!\n");
}

int main(void) {
	ps_init();
	run_all();
	ps_deinit();
	return 0;
}
