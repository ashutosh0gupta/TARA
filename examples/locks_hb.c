/* The following example from the r8169 driver illustrates the REORDER.RW pattern.   

There are 2 state variables involved in this example:

* IntrMask is a hardware register used to diable interrupts
* intr_mask is a software variable that mirrors the value of 
  IntrMask (for reasons related to performance and memory 
  coherency)*/

int IntrMask;
int init_finished;
int workqueue_items;
int interrupts;

void workqueue_thread() {
  assert (workqueue_items>= interrupts);
}

void irq_thread1() /*(interrupt thread):*/
{
  assume (IntrMask == 1);

  /* handle interrupt */
  assert(init_finished==1);
  int temp = workqueue_items;
  workqueue_items = temp + 1;
  interrupts += 1;
}

void irq_thread2() /*(interrupt thread):*/
{
  assume (IntrMask == 1);
  
  /* handle interrupt */
  assert(init_finished==1);
  int temp = workqueue_items;
  workqueue_items = temp + 1;
  interrupts += 1;
}

void startup_thread() /*(delayed interrupt handled):*/
{
        /* enable interrupts */
/*(!)*/     IntrMask = 1;
/*(!!)*/    init_finished = 1;
}

main() {
  IntrMask = 0;
  init_finished = 0;
  workqueue_items = 0;
  interrupts = 0;
  irq_thread1();
  irq_thread2();
  startup_thread();
  workqueue_thread();
}

/*
Here, writing the IntrMask variable releases the interrupt thread, 
which checks the intr_mask variable and handles the interrupt, if 
it is non-zero.

Correctness condition here is that the handled flag must eventually 
be set to 1 (i.e., all interrupts must be successfully handled).  
This condition is violated by the following execution:

thread1                        thread2

                                IntrMask = 1
if (intr_mask) --> false
                                intr_mask = 1;


Reordering the two lines labelled (!) and (!!) fixes the problem.

NOTE: In reality threads1 and 2 both run in a loop, where thread1 
releases thread2, which then reenables thread1, and so on.
*/
