#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
void assume( bool );
void assert( bool );
void fence();

int r1,r2,r3,r4,r5,r6,finished1 = 0,finished2 = 0;
int flag1 = 0, flag2 = 0;
int turn = 0;
int cs;

void* p0(void * arg) {

    flag1 = 1;
    r1 = flag2;
    if(r1 == 1)
    {
	r2 = turn;
	if( r2 == 0)
        {
	     r1 = flag2;
	     if( r1 != 1 )
	      {
		r3 = cs;
		cs = r3 + 1;
		//fence
		turn =1;
		flag1 = 0;
		//2nd iteration
		flag1 = 1;
	        r1 = flag2;
	        if(r1 == 1)
    		{
	            r2 = turn;
	            if( r2 == 0)
        	    {
		     	r1 = flag2;
		     	if( r1 != 1 )
		      	{
		            r3 = cs;
			    cs = r3 + 1;
			    //fence
			    turn =1;
			    flag1 = 0;
			    finished1=1;
		      	}
		      }
    		  }	
	      }
	  }
      }
    return NULL;
}

void* p1(void * arg) { 

    flag2 = 1;
    r4 = flag1;
    if(r4 == 1)
    {
	r5 = turn;
	if(r5 != 1)
        {
	    flag2 = 0;
	    r5 = turn; 
	   if(r5 == 1)
           {
	      flag2 = 1;
	      r4 = flag1;
    	      if(r4 != 1)
    	      {
	        r6 = cs;
	        cs = r6 + 1;
		//fence
                turn =0;
	        flag2 = 0;
		//2nd iteration
 		flag2 = 1;
	        r4 = flag1;
                if(r4 == 1)
    		{
	           r5 = turn;
	           if(r5 != 1)
        	   {
	              flag2 = 0;
	              r5 = turn; 
	              if(r5 == 1)
           	      {
  	                 flag2 = 1;
 	                 r4 = flag1;
     	                 if(r4 != 1)
    	                 {
	                    r6 = cs;
	                    cs = r6 + 1;
                            turn =0;
	                    flag2 = 0;
	                    finished2=1;
    	      		}		
    	   	      }
                   }
               }	        

    	     }	
    	  }
       }
   }
   return NULL;
}
int main() {
  __CPROVER_ASYNC_1: p0(0);
  p1(0);

  // pthread_t thr_0;
  // pthread_t thr_1;
  // pthread_create(&thr_0, NULL, p0, NULL );
  // pthread_create(&thr_1, NULL, p1, NULL );
  // pthread_join(thr_0, NULL);
  // pthread_join(thr_1, NULL);
  assert(!((finished1 == 1) && (finished2 == 1)) || cs == 4);
}
