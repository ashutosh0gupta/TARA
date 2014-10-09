#define USB_BUS_THREAD 2

/* framework variables */

int fw_tty_registered;
int fw_tty_initialized;
int fw_tty_lock;
int fw_driver_list_consistent;
int fw_idr_consistent;
int fw_table_lock;
int fw_serial_bus_lock;

/* belkin driver variables */

int drv_usb_registered;
int drv_usb_initialized;

int drv_registered_with_usb_fw;
int drv_registered_with_serial_fw;

int drv_id_table;
int drv_module_ref_cnt;
int drv_device_id_registered;

/* belkin per-device variables */
int dev_usb_serial_initialized;
int dev_autopm;
int dev_disconnected;
int dev_disc_lock;

/* belkin per-port variables */

int port_dev_registered;
int port_initialized;
int port_idr_registered;
int port_tty_installed;
int port_work;
int port_work_initialized;
int port_work_stop;
int port_tty_registered;
int port_consistent;
int port_lock;
int port_write_in_progress;

/* URB variables */
#define URB_IDLE      0
#define URB_SUBMITTED 1
int write_urb_state;

/*
 * Framework functions
 */
void usb_serial_init () {
    fw_tty_initialized = 1;
    fw_tty_registered = 1;
}


void usb_serial_exit () {
    
    fw_tty_registered = 0;
    
    //put_tty_driver(usb_serial_tty_driver);
    assume (port_tty_installed == 0);
    fw_tty_initialized = 0;
}

void lock_table () {
    atomicBegin();
    assume (fw_table_lock == 0);
    fw_table_lock = 1;
    atomicEnd();
}

void unlock_table () {
    fw_table_lock = 0;
}

void lock_serial_bus () {
    atomicBegin();
    assume(fw_serial_bus_lock == 0);
    fw_serial_bus_lock = 1;
    atomicEnd();
}

void unlock_serial_bus () {
    fw_serial_bus_lock-=1;
}

void lock_tty () {
    atomicBegin ();
    assume (fw_tty_lock == 0);
    fw_tty_lock = 1;
    atomicEnd ();
}

void unlock_tty () {
    fw_tty_lock = 0;
}

void lock_port () {
    atomicBegin ();
    assume (port_lock == 0);
    port_lock = 1;
    atomicEnd ();
}

void unlock_port () {
    port_lock = 0;
}

void usb_serial_probe () {
    lock_table();
    
    assert (fw_driver_list_consistent);
    if (/*(!drv_id_table) |*/ (!drv_registered_with_usb_fw)) {
        unlock_table();
    } else {

    try_module_get ();
    
    if (drv_module_ref_cnt <= 0) {
        unlock_table ();
    } else {

    assert (drv_usb_initialized);
    assert (drv_usb_registered);
   
    unlock_table();
    
    dev_usb_serial_initialized = 1;
    belkin_probe ();
    
    
    belkin_attach();
    
    /* Avoid race with tty_open and serial_install by setting the
     * disconnected flag and not clearing it until all ports have been
     * registered.
     */
    dev_disconnected = 1;
    
    // allocate_minors ()
    {
        lock_table();
        assert (fw_idr_consistent);
        fw_idr_consistent = 0;
        port_idr_registered = 1;
        fw_idr_consistent = 1;
        unlock_table();
    }
    
    port_work_initialized = 1;
    port_initialized = 1;
    port_dev_registered = 1;
    dev_disconnected = 0;
    
    // port_dev_registered = 1 increments module counter
    // module_put();
    }}
}


void usb_serial_disconnect () {
    lock_disc();
    /* must set a flag, to signal subdrivers */
    assert (dev_usb_serial_initialized>=0);
    dev_disconnected = 1;
    unlock_disc();
    
    assert (port_initialized);
    
    if (port_tty_installed) {

        //serial_hangup ();
        //serial_close ();
        atomicBegin ();
        assume (fw_tty_lock == 0);
        fw_tty_lock = USB_BUS_THREAD;
        atomicEnd ();

        assert (drv_module_ref_cnt > 0);
        assert (dev_usb_serial_initialized);
        assert (port_initialized);

        cancel_work_sync();
        port_work_initialized = 0;

        assume (port_write_in_progress == 0);
        port_tty_installed = 0;


        drv_module_ref_cnt-=1;
        usb_serial_put();
        // end: serial_close


        unlock_tty ();


        usb_serial_port_poison_urbs();
        //wake_up_interruptible(&port->port.delta_msr_wait);


        //if (port_dev_registered) 
        //    port_dev_registered = 0;
    }
    
    belkin_disconnect ();
    
    /* let the last holder of this object cause it to be cleaned up */
    usb_serial_put();
}

void cancel_work_sync () {
    port_work_stop = 1;

    atomicBegin();
    assume(port_work==0);
    atomicEnd();
}

void usb_serial_device_probe () {
    assert (port_initialized);
    assert (dev_usb_serial_initialized>=0);
    dev_autopm+=1;
    
    belkin_port_probe ();
    
    port_tty_registered = 1;
    
    dev_autopm-=1;
}

void usb_serial_device_remove () {
    assert (port_initialized);
    assert (dev_usb_serial_initialized>=0);
    
    /* make sure suspend/resume doesn't race against port_remove */
    dev_autopm+=1;
    
    port_tty_registered = 0;
    
    belkin_port_remove();
    
    dev_autopm-=1;
}

void usb_serial_put () {
    int old;
    
    assert (dev_usb_serial_initialized > 0);
    
    atomicBegin();
    old = dev_usb_serial_initialized;
    dev_usb_serial_initialized-=1;
    atomicEnd();
    
    if (old == 1) {
        //release_minors(serial);
        if (port_idr_registered)
        {
            lock_table();
            assert (fw_idr_consistent);
            fw_idr_consistent = 0;
            port_idr_registered = 0;
            fw_idr_consistent = 1;
            unlock_table(); 
        }
        
        belkin_release ();
        
        /* Now that nothing is using the ports, they can be freed */
        lock_serial_bus();
        port_dev_registered = 0;
        unlock_serial_bus();
        assume (port_tty_registered == 0);
        dev_usb_serial_initialized = -1;
        port_initialized = 0;
        drv_module_ref_cnt-=1;
    }
}

/*
 * Driver functions
 */

void try_module_get() {
    atomicBegin();
    if (drv_module_ref_cnt >= 0) 
        drv_module_ref_cnt+=1;
    atomicEnd();
}

void belkin_init () {
    /*
     * udriver must be registered before any of the serial drivers,
     * because the store_new_id() routine for the serial drivers (in
     * bus.c) probes udriver.
     *
     * Performance hack: We don't want udriver to be probed until
     * the serial drivers are registered, because the probe would
     * simply fail for lack of a matching serial driver.
     * So we leave udriver's id_table set to NULL until we are all set.
     *
     * Suspend/resume support is implemented in the usb-serial core,
     * so fill in the PM-related fields in udriver.
     */
    // TODO: implement new_id_store
    drv_usb_initialized = 1;
    drv_usb_registered = 1;

    // usb_serial_register()
    {
        lock_table();
        assert (fw_driver_list_consistent);
        fw_driver_list_consistent = 0;
        drv_registered_with_usb_fw = 1;
        fw_driver_list_consistent = 1;
        
        drv_registered_with_serial_fw = 1;
        unlock_table();
    }
 
    drv_id_table = 1;
}

void belkin_exit () {
    assert (drv_usb_initialized);
    drv_registered_with_usb_fw = 0;
    
    //usb_serial_deregister
    {
        lock_table ();
        
        assert(fw_driver_list_consistent);
        fw_driver_list_consistent = 0;
        drv_registered_with_usb_fw = 0;
        fw_driver_list_consistent = 1;
        
        drv_registered_with_serial_fw = 0;
        unlock_table();
    }
    
    drv_usb_registered = 0;
    drv_usb_initialized = 0;
}

/*
 * Per-device driver functions
 */
void belkin_probe () {
    // TODO
}

void belkin_attach () {
    // TODO
}

void belkin_disconnect () {
    // TODO
}

void belkin_release () {
    // TODO
}

void belkin_port_probe () {
    // TODO
}

void belkin_port_remove () {
    // TODO
}

void lock_disc () {
    atomicBegin();
    assume (dev_disc_lock == 0);
    dev_disc_lock = 1;
    atomicEnd();
}

void unlock_disc () {
    dev_disc_lock = 0;
}

/*
 * Per-port driver functions
 */

void usb_serial_port_poison_urbs() {
    // TODO
}

void serial_install () {

    // port = usb_serial_port_get_by_minor(idx);
    
    lock_table();
    if (!port_idr_registered) {
        unlock_table ();
        
    } else {


    lock_disc ();
	if (dev_disconnected) {
            unlock_disc ();
            unlock_table ();
	} else {
    assert (port_initialized);
    assert (dev_usb_serial_initialized > 0);
    dev_usb_serial_initialized+=1;

    unlock_table ();
    

    try_module_get ();
    if (drv_module_ref_cnt <= 0) {
        usb_serial_put ();
        unlock_disc ();
    } else {

    dev_autopm+=1;
    port_tty_installed = 1;
    unlock_disc ();
    }}}
}

void serial_hangup() {
    // TODO
    // takes tty lock
}

//void serial_close () {
//    assert (drv_module_ref_cnt > 0);
//    assert (dev_usb_serial_initialized);
//    assert (port_initialized);
//    assume (port_write_in_progress == 0);
//    port_tty_installed = 0;
//
//    drv_module_ref_cnt-=1;
//    usb_serial_put();
//}

void serial_write () {
    assert (port_initialized);
    assert (dev_usb_serial_initialized);
    assert (port_tty_installed);
    lock_port ();
    assert (port_consistent);
    port_consistent = 0;
    port_consistent = 1;
    unlock_port ();

    if (write_urb_state == URB_IDLE) {
        
    write_urb_state = URB_SUBMITTED;
    // TODO model concurrent operations on the port
    }
}

void serial_write_callback () {
    assert (port_initialized);
    lock_port ();
    assert (port_consistent);
    port_consistent = 0;
    port_consistent = 1;
    unlock_port ();
    assert (port_work_initialized);
    port_work = 1;
}

/*
 * Threads
 */
void fw_module_thread () {
    usb_serial_init();
    
    belkin_init();
    
    atomicBegin();
    assume(drv_module_ref_cnt == 0);
    drv_module_ref_cnt-=1;
    atomicEnd();
    
    belkin_exit();
    
    usb_serial_exit();
    // TODO: everything must be uninitialised
    
}

void usb_bus_thread () {
    assume (drv_usb_registered!=0 || drv_device_id_registered!=0);
    usb_serial_probe ();
    // TODO
    
    // hack to avoid checking return value of usb_serial_probe
    if (port_dev_registered)
        usb_serial_disconnect ();
}

void usb_cb_thread () {
    //while (drv_usb_registered != 0) {
        assume ((write_urb_state == URB_SUBMITTED) || (drv_usb_registered == 0));
        if (write_urb_state == URB_SUBMITTED) {
            assert (drv_usb_initialized);
            write_urb_state = URB_IDLE;
            serial_write_callback();
        }
    //}
}

void port_work_thread () {
    //while (port_work_active != 0) {
        assume ((port_work == 1) || (port_work_stop == 1));
        assert ((fw_tty_lock != USB_BUS_THREAD) || (port_work_stop == 0));
        lock_tty ();
        if (port_work) {
            assert (port_initialized);
            assert (port_tty_installed);
            port_write_in_progress = 0;
            port_work = 0;
        }
        unlock_tty ();
    //};
}

void serial_bus_thread () {
    lock_serial_bus();
    assume (port_dev_registered);
    usb_serial_device_probe ();
    unlock_serial_bus();
    
    assume (!port_dev_registered);
    lock_serial_bus();
    usb_serial_device_remove ();
    unlock_serial_bus();
}

void tty_thread () {
    assume (drv_registered_with_serial_fw);
    serial_install ();
    //while (port_tty_installed != 0) {
        lock_tty ();
        if (port_tty_installed) {
            port_write_in_progress = 1;
            //serial_write();
            assert (port_initialized);
            assert (dev_usb_serial_initialized);
            assert (port_tty_installed);
            lock_port ();
            assert (port_consistent);
            port_consistent = 0;
            port_consistent = 1;
            unlock_port ();

            if (write_urb_state == URB_IDLE)
                write_urb_state = URB_SUBMITTED;
            // end serial_write
        }
        unlock_tty ();
    //}
    // TODO
}

void attribute_thread () {
    try_module_get();

    if (drv_module_ref_cnt > 0) {
        assume (drv_registered_with_serial_fw);
        drv_device_id_registered = 1;
        drv_module_ref_cnt-=1;
    }
}

void main () {
    /* framework variables */
    
    fw_tty_registered = 0;
    fw_tty_initialized = 0;
    fw_tty_lock = 0;
    fw_driver_list_consistent = 1;
    fw_idr_consistent = 1;
    fw_table_lock = 0;
    fw_serial_bus_lock = 0;
    
    /* belkin driver variables */
    
    drv_usb_registered = 0;
    drv_usb_initialized = 0;
    
    drv_registered_with_usb_fw = 0;
    drv_registered_with_serial_fw = 0;
    
    drv_id_table = 0;
    drv_module_ref_cnt = 0;
    drv_device_id_registered = 0;
    
    /* belkin per-device variables */
    dev_usb_serial_initialized = -1;
    dev_autopm = 0;
    dev_disconnected = 0;
    dev_disc_lock = 0;
    
    /* belkin per-port variables */
    
    port_dev_registered = 0;
    port_initialized = 0;
    port_tty_registered = 0;
    port_tty_installed = 0;
    port_idr_registered = 0;
    port_work = 0;
    port_work_initialized = 0;
    port_work_stop = 0;
    port_consistent = 1;
    port_lock = 0;
    port_write_in_progress = 0;

    write_urb_state = URB_IDLE;
    
    /*******************/
    fw_module_thread ();
    usb_bus_thread ();
    serial_bus_thread ();
    tty_thread ();
    attribute_thread (); 
    usb_cb_thread ();
    port_work_thread ();
}
