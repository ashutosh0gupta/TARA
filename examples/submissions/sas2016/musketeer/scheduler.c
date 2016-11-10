volatile int current = 1;
volatile int Pone = 0;
volatile int Ptwo = 1;

void* th1(void* arg) {
int r0,r1,r2;
 
     r0 = Pone; 
     r1 = Ptwo;
     if(!((r0 == 0) && (r1 == 0)))
     {
	r2 = current;
	if (r2 == 1)
        {
	    Pone = 1;
	    Ptwo = 0;
	    current = 2;
	}
     }
     
}

void* th2(void* arg) {
int s0,s1,s2;
 
     s0 = Pone; 
     s1 = Ptwo;
     if(!((s0 == 0) && (s1 == 0)))
     {
	s2 = current;
	if (s2 == 2)
        {
	    Pone = 0;
	    Ptwo = 1;
	    current = 1;
	}
     }
     
}

void main() {
  __CPROVER_ASYNC_1: th1(0);
  __CPROVER_ASYNC_2: th2(0);
}
