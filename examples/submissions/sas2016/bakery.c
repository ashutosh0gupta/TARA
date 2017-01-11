
int c1, c2, n1, n2;

int p1() {
  c1 = 1;
  int r1 = n1;
  int r2 = n2;
  int r3 = r2 > r1 ? r2+1 : r1+1;
  n1 = r3;
  c1 = 0;
  while( c2 ) {
    while( n2 != 0 && n2 < n1 );
  }
  cs = cs + 1;

  n1 = 0;
}


int p1() {
  c2 = 1;
  int r1 = n1;
  int r2 = n2;
  int r3 = r2 > r1 ? r2+1 : r1+1;
  n2 = r3;
  c2 = 0;
  while( c1 ) {
    while( n1 != 0 && n1 <= n2 );
  }
  cs = cs + 1;

  n2 = 0;
}
