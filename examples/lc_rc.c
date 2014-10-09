/* Based on 58be81ed301d96045bca2b85f3b838910efcfde4
 * (this patch actually fixes several related races; we only model one of them here)
 *
 * The essence of this example is that there are three worker threads that depend on each other.
 * The threads must be killed in the right order to avoid dangling references.
 * 
 * Race in the original implementation: 1->i->ii->2->P(assertion failure)
 * There are many instruction reorderings that fix this race, e.g., 5;6;1;2;3;4, but
 * it introduces a new race: 5->P->Q->R->a (assertion failure).
 * The only correct rearrangement is: 3->4->5->6->1->2
 */

int neh_running;
int stop_neh;

int rc_running;
int stop_rc;

int rsv_running;
int stop_rsv;


// this thread sends stop signals to each of the other three threads
void shutdown_thread () {
    // tell workers to stop and wait for the to terminate
    // 1.
    stop_rsv = 1;
    // 2.
    assume (!rsv_running);
    // 3.
    stop_rc = 1;
    // 4.
    assume (!rc_running);
    // 5.
    stop_neh = 1;
    // 6.
    assume (!neh_running);
}

rc_thread () {
    // a.
    assert (neh_running);
    // b.
    assume (stop_rc);
    // c.
    rc_running = 0;
}

neh_thread () {
    // P.
    assert (rsv_running);
    // Q.
    assume (stop_neh);
    // R.
    neh_running = 0;
}

rsv_thread () {
    // i
    assume (stop_rsv);
    // ii
    rsv_running = 0;
}

void main () {
    neh_running = 1;
    stop_neh = 0;

    rc_running = 1;
    stop_rc = 0;

    rsv_running = 1;
    stop_rsv = 0;

    shutdown_thread ();
    rc_thread ();
    neh_thread ();
    rsv_thread ();
};
