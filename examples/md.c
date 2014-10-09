/* Based on a7a3f08dc24690ae00c75cfe4b4701a970dd0155
 *
 * This example requires the use of the flush_workqueue() barrier to wait for an async 
 * activity to complete.
 *
 * Race in the original implementation:
 *
 * 1->2->3->P->Q->R->S (assertion failure)
 *
 * The developers fixed the race by adding the line labelled with "!!!", which waits for the
 * work queue to complete processing device removal
 *
 */

int lock1;
int sysfs_entry;
int work;
int removed;
int wait_for_workqueue;

void mddev_put() {
    // 2. complete request asynchronously, as this function is not allowed to block
    work = 1;
}

void add_new_disk() {
    // need a barrier here:
    // !!! flush_workqueue(i);

    // R. lock mutex
    atomicBegin();
    assume(lock1 == 0);
    lock1 = 1;
    atomicEnd();

    // putting the barrier here causes deadlock

    // S.disk removal must be finished by now
    assert (sysfs_entry == 0);
    // T.
    sysfs_entry = 1;

    // U.
    lock1 = 0;
}

void flush_workqueue() {
    noReorderBegin ();
    // V.
    wait_for_workqueue = 1;
    // W.
    assume (work == 0);
    // X.
    wait_for_workqueue = 0;
    noReorderEnd ();
}

void remove_thread () {
    // 1. remove disk
    mddev_put();
    // 3.
    removed = 1;
}

void add_thread () {
    // P.
    assume (removed == 1);
    // Q.
    add_new_disk ();
}

// workqueue thread that completes the disk remove task
void md_misc_wq_thread() {
    noReorderBegin();

    // a. wait for a work item to be queued
    assume(work == 1);
    // b. remove sysfs entry
    sysfs_entry = 0;

    // c. under some circumstances, this code may need to take the lock
    if (nondet) {
        
        // d.
        atomicBegin();
        assume(lock1 == 0);
        lock1 = 1;
        atomicEnd();
        // e.
        lock1 = 0;
    };
    // f.
    work = 0;

    noReorderEnd();
}


void main () {
    lock1 = 0;
    sysfs_entry = 1;
    work = 0;
    removed = 0;
    wait_for_workqueue = 0;

    add_thread ();
    remove_thread ();
    md_misc_wq_thread ();
}
