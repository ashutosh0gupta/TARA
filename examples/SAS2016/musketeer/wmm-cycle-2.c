volatile int x,y,w,z;

void* th1(void* arg) {
  x = 1;
  w = 1;
  y = 1;
  z = 1;
}

void* th2(void* arg) {
  int r1,r2,r3,r4;
  r1 = y;
  r2 = x;
  r3 = z;
  r4 = w;
}

void main() {
  __CPROVER_ASYNC_1: th1(0);
  th2(0);
}
