/* Based on 13c76aba7846647f86d479293ae0a0adc1ca840a
 *
 * Race: 1->2->3->a->b->c
 * Fix: add statenment labelled "!!!" (which happens to be 
 * implemented by acquiring and instantly releasing the lock)
 */

#define R_SECONDARY 1
#define R_PRIMARY   2

int lock1;
int STATE_SENT;
int old_role;
int role;
int old_uuid;
int uuid;
int sent_uuid;

drbd_set_role_thread () {
    // 1.
    atomicBegin();
    assume (lock1 == 0);
    lock1 = 1;
    atomicEnd();

    // 2.
    if (STATE_SENT==0) {
        // 3.
        role = R_PRIMARY;
        // 4.
        uuid = uuid+1;
    };

    // 5.
    lock1 = 0;
}

drbd_connect_thread () {
    // a.
    STATE_SENT = 1;

    // !!! wait (lock1 == 0);

    // c.
    assert ((uuid != old_uuid));

    // d.
    STATE_SENT = 0;
}

void main() {
    role = R_SECONDARY;
    uuid = nondet_int();
    old_role = role;
    old_uuid = uuid;

    lock1 = 0;
    STATE_SENT = 0;

    drbd_set_role_thread ();
    drbd_connect_thread ();
}
