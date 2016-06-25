void assume( bool );
void assert( bool );

int m;
int x, y, z, balance;
int deposit_done=0, withdraw_done=0;

void deposit_thread() 
{
  int l = balance;
  balance = l + y;
  deposit_done=1;

}

void withdraw_thread() 
{
  int l2 = balance;
  balance = l2 - z;
  withdraw_done = 1;
}

void check_result_thread() 
{
  assume (deposit_done==1 && withdraw_done==1);
  assert(balance == (x + y) - z); 
}

void main() 
{

  x = 20;
  y = 15;
  z = 6;
  balance = x;
  deposit_done = 0;
  withdraw_done = 0;

  check_result_thread();
  deposit_thread();
  withdraw_thread();
}
