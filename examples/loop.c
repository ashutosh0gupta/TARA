int sum;
int f2;

void thread1() {
  int i = 0;
  while(nondet) {
    assume (i<10);
    sum = sum + 1;
    i = i + 1;
  }
  assume(i>=10);
  int temp = sum;
  sum = temp + 1;
  assume (f2 > 0);
  assert (sum > 11);
}

void thread2() {
  sum = sum + 1;
  f2 = 1;
}

void main() {
  sum = 0;
  f2 = 0;
  thread1();
  thread2();
}