
float arr[4];
float sum;

int f1,f2;

void compute_thread1()
{ 
  float lsum = 0;
  lsum = lsum + arr[0];
  lsum = lsum + arr[1];
  float temp = sum;
  sum = temp + lsum;
  
  arr[0] /= sum;
  arr[1] /= sum;
  
  f1 = 1;
}


void compute_thread2()
{  
  float lsum = 0;
  lsum = lsum + arr[2];
  lsum = lsum + arr[3];
  float temp = sum;
  sum = temp + lsum;
  
  arr[2] /= sum;
  arr[3] /= sum;
  
  f2 = 1;
}


void check_thread() {
  assume(f1>0 && f2>0 ); // make sure all threads completed
  assert(arr[0] + arr[1] + arr[2] + arr[3] == 1 );
}

void main()
{
  f1 = 0;
  f2 = 0;
  sum = 0;
  
  arr[0] = 1;
  arr[1] = 2;
  arr[2] = 4;
  arr[3] = 8;
  
  compute_thread1();
  compute_thread2();
  check_thread();
}
