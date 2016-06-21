int g1,g2,g3,g4,g5; // All are itegers

//----------------------------------------------------------------------------
// TODO: remove this
void assume( bool b );
void assert( bool b );


int bar() {
  int x = g1;
  if( g2 < 0 ) {
    int y = g4;
    x++;
  }else{
    int y = g5;
    x--;
  }
  g3 = x;
  return x;
}

//----------------------------------------------------------------------------
// int foo(int x, int y ) {
//   /* int result = x > 61 ? x : (x / 17); */
//   return x;
// }

