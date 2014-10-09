/*
 * This example shows a define-use bug.  The pci_thread is responsible
 * for setting the pointer to the hw_start function and 
 * signals that the device is registered with the "registered" variable.
 * 
 * The network interface thread calls drv_open once the network device
 * is registered. The drv_open dereferences the function pointers.
 * 
 * This is a define-use bug because the pointer can be dereferenced before 
 * it is initialised.
 * 
 * In our encoding we model the pointer as an integer variable that is assigned
 * a value when initialised (it is initially NULL).
 */

int hw_start;
int registered = 0;

void drv_hw_start() {
  // does something
}

void register_netdev()
{
  registered = 1;
}

void pci_thread() {
  register_netdev();
  hw_start = 123456789;
}

void drv_open() {
  assert (hw_start > 0);
}

void network_thread()
{

    assume (registered != 0)
      drv_open();

}

void main() {
  hw_start = 0;
  registered = 0;
  pci_thread();
  network_thread();
}