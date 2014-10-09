/*  This is a skeleton of the rtl8169 driver.  It contains only code
    related to synchronisation, including all the threads found
    in the real driver (seven threads overall).  It contains 5 
    concurrency bugs and one deadlock bug. 5 bugs can be fixed 
    using the REORDER pattern and 1 requires an atomic section.
    We learn about the atomic section using the good traces

Good trace analysis
===================
We obtain many good traces, but this one teaches us a crucial piece of information.

irq_thread                                      napi_thread
----------                                      -----------



if (irq_enabled && IntrStatus && IntrMask)
assert(intr_mask!=0)
|- drv_irq
   |- write_IntrMask(0);
   |- intr_mask = 0;
   |- napi_schedule ();
                                                |- drv_napi_poll()
                                                   |- intr_mask = 255;
                                                   |- write_IntrMask(255); [inlined]

if (irq_enabled && IntrStatus && IntrMask)
assert(intr_mask!=0)
|- drv_irq
   |- (...)
                         
From this trace we learn that the order of intr_mask = 255 and write_IntrMask(255)
is important because otherwise we could pass through the if and the assert would
fail.

Bug fixing
==========
1. Race between driver initialisation function (drv_probe), and network interface
initialisation (drv_open)

pci_thread                                      network_thread
----------                                      --------------

|- drv_probe()
   |- register_netdev()
      |- registered = true
                                                |- open
                                                   |- registered == true
                                                   |- drv_open()
                                                      |- (*hw_start)(); // uninitialised pointer
   |- hw_start = drv_hw_start

Possible solution: swap register_netdev() and (hw_start = drv_hw_start) in drv_probe


2. Race condition between drv_up and drv_irq running in the irq_thread.


network_thread                    irq_thread
--------------                    ----------------

|- open
  |- drv_open
     |- dev_up
        |- write_IntrMask(1);
								   |if (IntrMask)
                                   |-drv_irq()
								     | assert (intr_mask) !!!

		|- intr_mask = 1;
		
Possible solution: swap write_IntrMask(1) and intr_mask=1 in the body of dev_up

                                       
3.  Race between interrupt handler and NAPI thread

irq_thread                                      napi_thread
----------                                      -----------

                                                |- drv_napi_poll()
(irq_enabled && IntrStatus && IntrMask)
assert(intr_mask!=0)
|- drv_irq
   |- write_IntrMask(0);
                                                   |- intr_mask = 255;
   |- intr_mask = 0;
   |- napi_schedule ();
                                                   |- write_IntrMask(255); [inlined]
(irq_enabled && IntrStatus && IntrMask)
assert(intr_mask!=0)  !!!


With the insight we gained from good trace 1 (above) we know not to reorder write_IntrMask(255)
and (intr_mask = 255) in the body of drv_napi_poll(). An atomic section solves the problem


4. Race condition between drv_clone and drv_irq running in the irq_thread.


network_thread                    irq_thread
--------------                    ----------------

|- close()
  |- drv_close()
     |- dev_down()
        |- dev_on = false
                                   |if (irq_enabled && IntrStatus && IntrMask)
                                   |-drv_irq()
                                     | write_IntrMask
                                       |- assert(dev_on) !!!

      |- free_irq()
        |- irq_enabled = 0;
        
Possible solution: swap dev_down and free_irq in the body of drv_close.


5. Race between network_thread and napi_thread

network_thread                                  napi_thread
--------------                                  -----------

|- close()
   |- drv_close()
      |- dev_down()
         |- dev_on = false
                                                |- drv_napi_poll
                                                   |- write_IntrMask(1);
                                                      |- assert(dev_on)  !!!
      |- napi_disable()

Possible solution: swap dev_down and napi_disable in the body of drv_close


6. Deadlock in sysfs_thread

sysfs_thread                                    pci_thread                             
------------                                    ----------

|- lock(sysfs_lock)
                                                |- lock(dev_lock)
                                                |- drv_disconnect
                                                   |- remove_sysfs_files
                                                      |- lock(sysfs_lock);  // DEADLOCK
|- lock(dev_lock)                               
                                                        
Possible solution: change the order the sysfs_thread locks sysfs_lock and dev_lock

*/



typedef unsigned char u8;

typedef int bool;
#define true  1
#define false 0

typedef int mutex_t;

#define locked   1
#define unlocked 0

typedef int semaphore_t;

#define PCI_THREAD 5
#define SYSFS_THREAD 6
int want_sysfs = 0;
int want_dev = 0;

/* Forward declarations of driver methods */
int  drv_open();
void drv_close();
void drv_disconnect();
int  drv_irq();
void drv_napi_poll();
void drv_start_xmit();
void drv_xmit_timeout();
void drv_reset_task();
unsigned int drv_sysfs_read(int off);

/**************************************************************
 * Environment model.  
 **************************************************************/

/* Global state */
bool registered       = false; // true when the driver interface is open by the OS
bool netif_running    = false; // true after the driver has been registered with the TCP/IP stack

bool irq_enabled      = false;

bool napi_enabled     = false;
bool napi_scheduled   = false;

bool reset_task_ready = false;

bool sysfs_registered = false;

mutex_t dev_lock      = unlocked;
mutex_t sysfs_lock    = unlocked;
mutex_t rtnl_lock     = unlocked;

semaphore_t irq_sem   = 1;
semaphore_t napi_sem  = 1;

u8 IntrStatus;
u8 IntrMask;

/**************************************************************
 * Driver definitions from below  
 **************************************************************/

u8 intr_mask;
bool dev_on    = false;

#define budget 100

void dev_up();
void dev_down();
int  handle_interrupt();
void create_sysfs_files();
void remove_sysfs_files();
void write_IntrMask(u8 val);
void write_IntrStatus(u8 val);
void napi_schedule();
void device_remove_bin_file();
void napi_enable();
void synchronize_irq();

int hw_start;

/* PCI thread: PCI bus probe, shutdown, and unplug operations 
 */
void pci_thread()
{
	int probe1;
	probe1 = drv_probe();
    if (!probe1) {
        // lock(dev_lock)
        noReorderBegin();
        want_dev = PCI_THREAD;
        atomicBegin();
        assume(dev_lock==0);
        dev_lock = PCI_THREAD;
        want_dev = 0;
        atomicEnd();
        noReorderEnd();
        
		drv_disconnect();
		unlock(dev_lock);
	}
}

/* Configuration thread
 */
void network_thread() 
{
	int open1;
    while (1) {
		open1 = open();
        if (open1==0){

            if (nondet)
                drv_start_xmit();
            else 
                drv_xmit_timeout();

			close();
		};
    };
}

int open()
{
    int ret1 = 0;

    lock(rtnl_lock);
    if (registered != 0) {
        ret1 = drv_open();
		if (ret1 == 0) {
            netif_running = true;
        }
    };
    unlock(rtnl_lock);
    return ret1;
}

int close()
{
    lock(rtnl_lock);
    if (netif_running != 0) {
        netif_running = false;
        drv_close();
    };
    unlock(rtnl_lock);
}

/* Interrupt thread.  Contains a very coarse model of how the device 
 * generates async interrupts, calls the irq entry point in a loop. 
 */
void irq_thread()
{
    while (1) { 

       down(irq_sem);
       if (irq_enabled!=0 && IntrStatus!=0 && IntrMask!=0) {
           assert(intr_mask != 0);
           drv_irq();
       };
       up(irq_sem);
   };
}

void dev_thread()
{
	while (1) {
        IntrStatus = 1;
	}
}

/* NAPI thread: delayed interrupt handling. 
 */

void napi_thread()
{

    while(1) {
        down(napi_sem);
        if (napi_scheduled) {
            drv_napi_poll();
        }
        up(napi_sem);
    };
}

/* Workqueue thread */
void workqueue_thread()
{
    while(1) {
        if (reset_task_ready) {
            drv_reset_task();
            reset_task_ready = false;
        };
    };
}

/* SysFS thread: user thread that reads driver configuration settings. */
void sysfs_thread()
{
	int nondet1;
    while (1) {
        // lock(sysfs_lock)
        noReorderBegin();
        want_sysfs = SYSFS_THREAD;
        atomicBegin();
        assume(sysfs_lock==0);
        sysfs_lock = SYSFS_THREAD;
        want_sysfs = 0;
        atomicEnd();
        noReorderEnd();
        
        // lock(dev_lock)
        noReorderBegin();
        want_dev = SYSFS_THREAD;
        atomicBegin();
        assume(dev_lock==0);
        dev_lock = SYSFS_THREAD;
        want_dev = 0;
        atomicEnd();
        noReorderEnd();
        
        if (sysfs_registered != 0) {
			nondet1 = nondet_int();
            drv_sysfs_read(nondet1);
        };
        unlock(dev_lock);
        unlock(sysfs_lock);
    };
}

/* OS callbacks invoked by the driver
 */

int register_netdev()
{
    int ret2;
    if (nondet) {
        registered = true;
        ret2 = 0;
    } else {
        ret2 = -1;
    }
    return ret2;
}

void unregister_netdev()
{
    registered = false;
    while (netif_running != 0) {};
}

int request_irq()
{
    int ret3;
    if (nondet) {
        irq_enabled = true;
        ret3 = 0;
    } else {
        ret3 = -1;
    }
    return ret3;
}

void free_irq()
{
    irq_enabled = false;
}

void napi_enable()
{
    napi_enabled = true;
}

void napi_disable()
{
    napi_enabled = false;
    down(napi_sem);
}

void napi_schedule()
{
    atomicBegin();
    if (napi_enabled!=0)
        napi_scheduled = true;
    atomicEnd();
}

void napi_complete()
{
    napi_scheduled = false;
}
 

void synchronize_irq()
{
    down(irq_sem);
    up(irq_sem);
}

void device_create_bin_file()
{
    sysfs_registered = true;
}

void device_remove_bin_file()
{
    sysfs_registered = false;
}

/***************************************************************
 * Driver code
 ***************************************************************/


int drv_probe()
{
    int rc1;
    int ret4;

    create_sysfs_files();

    rc1 = register_netdev();
	hw_start = 1;
	
    if (rc1 < 0) {
        ret4 = rc1;
    } else {
        ret4 = 0;
    }
    return ret4;
}

int drv_open()
{
    int rc2;
    int ret5;

    assert (hw_start);

    IntrMask = 0;

    rc2 = request_irq();
	
    if (rc2 < 0)
        ret5 = rc2;
    else {
        napi_enable();
        dev_up();
        ret5 = 0;
    }
    return ret5;
}

void drv_close()
{
	dev_down();
    napi_disable();
	free_irq();
}

void drv_disconnect()
{
    unregister_netdev();
    remove_sysfs_files();
}

int drv_irq()
{
    u8 status1;
    int handled2 = 0;

    status1 = IntrStatus;
    while (nondet != 0) {
        if (intr_mask != 0) {
            write_IntrMask(0);
            intr_mask = 0;
            napi_schedule ();
            handled2 = 1;
        };
        write_IntrStatus(status1);
        status1 = IntrStatus;
    }
    assume(!status1);
    return handled2;
}

void drv_napi_poll()
{
    int work_done;

    work_done = handle_interrupt();

    if (work_done < budget) {
        napi_complete();
        intr_mask = 0xff;
        IntrMask = 0xff;
    }
}

void drv_start_xmit()
{
}
                
void drv_xmit_timeout()
{
    reset_task_ready = true;
}

void drv_reset_task()
{
	lock(rtnl_lock);
    // Skip the reset sequence if a racing rtl8169_down disabled 
    // the device. 
    if (netif_running){

        // wait for in-progress send and receive operations to finish.
        napi_disable();

        // reset hardware
        dev_down();
        dev_up();

        napi_enable();
        unlock(rtnl_lock);
    }
}

unsigned int drv_sysfs_read(int off)
{
	int nondet2;
    nondet2 = nondet_int();
    return nondet2;
};

void write_IntrMask(u8 val)
{
    assert(dev_on!=0);
    IntrMask = val;
}

void write_IntrStatus(u8 val)
{
    assert(dev_on!=0);
    IntrStatus = val;
}

void dev_up()
{
    dev_on = true;
    write_IntrMask(1);
	intr_mask = 1;
};

// shutdown device hardware
void dev_down()
{
    write_IntrMask(0);

    synchronize_irq();

    dev_on = false;
};


int handle_interrupt()
{
	int nondet3;
    nondet3 = nondet_int();
    IntrStatus = 0;
    return nondet3;
}

void create_sysfs_files()
{
    lock(sysfs_lock);
    device_create_bin_file();
    unlock(sysfs_lock);
}

void remove_sysfs_files()
{
    // lock(sysfs_lock)
    noReorderBegin();
    want_sysfs = PCI_THREAD;
    atomicBegin();
    assume(sysfs_lock == 0);
    sysfs_lock = PCI_THREAD;
    want_sysfs = 0;
    atomicEnd();
    noReorderEnd();
    
    device_remove_bin_file();
    unlock(sysfs_lock);
}

// thread checks if we deadlock (this thread can be scheduled whenever convenient to prove a deadlock)
void deadlock_thread() {
    assert(!(dev_lock == want_sysfs && sysfs_lock == want_dev && dev_lock != 0 & sysfs_lock != 0));
}

void main() {
	registered       = false; // true when the driver interface is open by the OS
	netif_running    = false; // true after the driver has been registered with the TCP/IP stack

	irq_enabled      = false;

	napi_enabled     = false;
	napi_scheduled   = false;

	reset_task_ready = false;

	sysfs_registered = false;

	dev_lock      = unlocked;
	sysfs_lock    = unlocked;
	rtnl_lock     = unlocked;

    want_sysfs = 0;
    want_dev = 0;
    
	irq_sem   = 0;
	napi_sem  = 0;
	dev_on    = false;
    
    IntrStatus = 0;
    IntrMask = 0;
    intr_mask = 0;
    
    hw_start = false;
	
	pci_thread();
	network_thread();
	irq_thread();
	napi_thread();
	workqueue_thread();
	sysfs_thread();
	dev_thread();
    deadlock_thread();
}
