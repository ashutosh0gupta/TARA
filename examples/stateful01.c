int data1, data2;

int ma, mb;

int f1,f2;

void thread1()
{  
  lock(ma);
  data1=data1+1;
  unlock(ma);

  lock(ma);
  data2=data2+1;
  unlock(ma);
  f1 = 1;
}


void thread2()
{  
  lock(ma);
  data1+=5;
  unlock(ma);

  lock(ma);
  data2-=6;
  unlock(ma);
  f2 = 1;
}

void thread3()
{
  //assume (f1 == 1);
  //assume (f2 == 1);
  //assert (data1!=16 || data2!=5);
  assert (data1==16 || data2==5);
}

void main()
{
  ma = 0;
  mb = 0;
  f1 = 0;
  f2 = 0;

  data1 = 10;
  data2 = 10;

  thread1();
  thread2();
  thread3(); 

}
