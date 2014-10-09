int a,b;

void set_thread()
{
  a = 1;
  b = -1;
}


void check_thread()
{
  assert((a==0&&b==0) || (a==1&&b==-1));
}


void main()
{
  a = 0;
  b = 0;
  
  set_thread();
  check_thread();
  
}