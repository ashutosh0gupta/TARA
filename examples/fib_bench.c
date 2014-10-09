int i;
int j;
int f1,f2;


thread1() {
    int c = 0;
    while(nondet) {
      assume(c<5);
      i=i+j;
      c = c+1;
    }
    assume (c >= 5);
    f2 = 1;
}

thread2() {
  int c = 0;
  while(nondet) {
    assume(c<5);
    j=j+i;
    c = c+1;
  }
  assume (c >= 5);
    f1 = 1;
}

thread3() {
    assume(f1==1);
    assume(f2==1);
    assert(i < 144 && j < 144);
}

void main () {
    i = 1;
    j = 1;
    f1 = 1;
    f2 = 1;
    
    thread1 ();
    thread2 ();
    thread3 ();
}