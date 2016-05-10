volatile int x,y,z;

void* th1(void* arg) {
int r1,r2;
  x = 2;
 r1 = z;
  x = 0;
  r2 = y;
}

void* th2(void* arg) {
int r3,r4;
  y = 0;
  r3 = z;
  y = 1;
  r4 = x;
}

void main() {
  __CPROVER_ASYNC_1: th1(0);
  __CPROVER_ASYNC_2: th2(0);
}
