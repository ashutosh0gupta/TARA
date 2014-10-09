int w=0, r=0, x, y;


void thread1a() { //writer
    atomicBegin();
    assume(w==0 && r==0);
    w = 1;
    atomicEnd();
    x = 3;
    w = 0;
}

void thread1b() { //writer
    atomicBegin();
    assume(w==0 && r==0);
    w = 1;
    atomicEnd();
    x = 3;
    w = 0;
}

void thread2a() { //reader
  int l;
  atomicBegin();
  assume(w==0);
  r = r+1;
  atomicEnd();
  l = x;
  y = l;
  assert(y == x);
  l = r-1;
  r = l;
}

void thread2b() { //reader
    int l;
    atomicBegin();
    assume(w==0);
    r = r+1;
    atomicEnd();
    l = x;
    y = l;
    assert(y == x);
    l = r-1;
    r = l;
}

void main() {
    w = 0;
    r = 0;
    x = 0;
    y = 0;
thread1a();
thread2a();
thread1b();
thread2b();
}
