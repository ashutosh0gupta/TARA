int turn; // integer variable to hold the ID of the thread whose turn is it
int flag1 = 0, flag2 = 0; // boolean flags

void* thr1(void * arg) { // frontend produces 12 transitions from this thread. It would be better if it would produce only 8!
  flag1 = 1;
  turn = 1;
  do {} while (flag2==1 && turn==1);
  // begin: critical section
  // end: critical section
  flag1 = 0;
}

void* thr2(void * arg) {
  flag2 = 1;
  turn = 0;
  do {} while (flag1==1 && turn==0);
  // begin: critical section
  // end: critical section
  flag2 = 0;
}
  
int main()
{
  __CPROVER_ASYNC_1: thr1(0);
  thr2(0);
}
