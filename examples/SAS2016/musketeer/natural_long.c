volatile int x1 = 0;
volatile int x2 = 0;
volatile int x3 = 0;
volatile int x4 = 0;
volatile int x5 = 0;
volatile int x6 = 0;
volatile int x7 = 0;
volatile int x8 = 0;
volatile int x9 = 0;
volatile int x10 = 0;
volatile int check = 0;


void* th1(void* arg) {
x1 = 1;
x2 = 1;
x3 = 1;
x4 = 1;
x5 = 1;
x6 = 1;
x7 = 1;
x8 = 1;
x9 = 1;
x10 = 1;
}

void* th2(void* arg) {
  		int s1;
  		int s2;	
		int s3;
		int s4;
		int s5;
		int s6;	
		int s7;
		int s8;
		int s9;
		int s10;
		
		s10 = x10;
		s9 = x9;
		s8 = x8;
		s7 = x7;
		s6 = x6;
		s5 = x5;
		s4 = x4;
		s3 = x3;
		s2 = x2;
		s1 = x1;	
		
}

void main() {
  __CPROVER_ASYNC_1: th1(0);
  th2(0);
}


