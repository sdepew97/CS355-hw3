/* check_llist.c

   Unit tests for llists
*/

#include <check.h>

#include "llist.h"

START_TEST(test_push_pop)
{
  llist l = llist_new();
  
  ck_assert_int_eq(llist_size(l), 0);

  llist_push(l, 3);
  llist_push(l, 4);

  ck_assert_int_eq(llist_pop(l), 4);

  llist_push(l, 5);
  ck_assert_int_eq(llist_peek(l), 5);
  llist_push(l, 6);

  ck_assert_int_eq(llist_pop(l), 6);
  ck_assert_int_eq(llist_size(l), 2);
  ck_assert_int_eq(llist_pop(l), 5);
  ck_assert_int_eq(llist_peek(l), 3);
  ck_assert_int_eq(llist_pop(l), 3);
  ck_assert_int_eq(llist_size(l), 0);

  llist_free(l);
}
END_TEST

START_TEST(test_interior)
{
  llist l = llist_new();

  llist_insert(l, 0, 3);
  llist_insert(l, 0, 2);
  llist_insert(l, 0, 1);
  llist_insert(l, 1, 8);
  llist_insert(l, 4, 10);
  llist_insert(l, 1, 6);

  // should be [1, 6, 8, 2, 3, 10]
  ck_assert_int_eq(llist_size(l), 6);
  ck_assert_int_eq(llist_get(l, 2), 8);
  ck_assert_int_eq(llist_get(l, 5), 10);
  ck_assert_int_eq(llist_get(l, 0), 1);

  ck_assert_int_eq(llist_set(l, 2, 42), 8);
  ck_assert_int_eq(llist_set(l, 5, 88), 10);
  ck_assert_int_eq(llist_set(l, 0, 33), 1);

  ck_assert_int_eq(llist_remove(l, 1), 6);
  ck_assert_int_eq(llist_remove(l, 0), 33);
  ck_assert_int_eq(llist_remove(l, 3), 88);

  ck_assert_int_eq(llist_size(l), 3);
  ck_assert_int_eq(llist_peek(l), 42);

  llist_free(l);
}
END_TEST

// the main() function for unit testing is fairly prescribed.
// Just copy & paste, but make sure to update the test names!
int main()
{
  // `check` allows for multiple test suites, but we'll always
  // just have one, called "main"
  Suite* s = suite_create("main");

  // Inside a suite are potentially many `TCase`s.
  TCase* tc = tcase_create("llist");

  // Each TCase can have many individual testing functions.
  tcase_add_test(tc, test_push_pop);
  tcase_add_test(tc, test_interior);

  // Having set up the TCase, add it to the suite:
  suite_add_tcase(s, tc);

  // To run the tests, we need a runner:
  SRunner* sr = srunner_create(s);

  // This next line means that all tests run sequentially (instead
  // of at the same time). It also means that valgrind's output is
  // simpler.
  srunner_set_fork_status(sr, CK_NOFORK);

  // You can see options for running only some of the tests at
  // the documentation link above. The CK_NORMAL indicates to
  // print only failing tests.
  srunner_run_all(sr, CK_NORMAL);

  // after running, we can retrieve the number of failures
  int number_failed = srunner_ntests_failed(sr);

  // We're done now with the testsuite, so dispose of it properly
  srunner_free(sr);

  // conveniently, number_failed makes a nice exit code.
  // a non-zero answer means a failed test!
  return number_failed;
}
				    
