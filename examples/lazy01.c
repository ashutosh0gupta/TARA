
int data = 0;
int mutex;

void thread1()
{
  lock(mutex);
  data=data+1;
  unlock(mutex);
}


void thread2()
{
  lock(mutex);
  data=data+2;
  unlock(mutex);
}


void thread3()
{
  lock(mutex);
  assert (data < 3);
  unlock(mutex);    
}


void main()
{
  data = 0;
  mutex = 0;
  
  thread1();
  thread2();
  thread3();
  
}