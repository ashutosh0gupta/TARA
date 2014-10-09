/* An extremely simplified version of 818a557eeb9c16a9a2dc93df348b0ff68fbc487f
 * The driver developer fixed the problem by adding locks.  It could also be solved
 * using reordering; however a naive reordering introduces a new race.
 *
 * Scenario: the driver must add the device to its internal list and then register
 * it with the client.  After the device has been registered, the client starts 
 * using it; however this will fail if the device has not been added to the list.
 * It will also fail if the device was added to the list, but the list entry has not
 * been properly initialised.  
 */


// List of registered devices.  Model one element only and use a flag to indicate when the list is empty.
int devlist_nonempty;
// both fields must be set to valid values before registering the device
int vdev;     
int vbi_dev; 


// device has been registered with the client
int registered;

// Thread 1: driver initialisation thread

void init_thread() {
    // 1. add new device to list
    devlist_nonempty = 1;

    // 2. initialise vdev
    vdev = 1;

    // 3. register vdev
    registered = 1;

    // 4. initialise vbi_dev
    vbi_dev = 1;

    // race in the above implementation: 1 -> 2 -> 3 -> A -> B -> C (assertion violation)
    // possible solution: reorder 2; 3; 4; 1; introduces new race: 2 -> 3 -> A (assertion violation)
    // correct sequence: 2; 4; 1; 3;
}

// Thread 2: client thread
void client_thread () {
    // wait for the driver to register the device
    assume (registered);

    // A
    assert (devlist_nonempty);

    // B
    assert (vdev != -1);

    // C
    assert (vbi_dev != -1);
}

void main () {
    devlist_nonempty = 0;
    vdev = -1;
    vbi_dev = -1;
    registered = 0;

    init_thread();
    client_thread();
}
