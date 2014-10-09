int page_table[];
int memory[];

int get_data(int page) {
  int loc = page_table[page];
  int data = memory[loc];
  return data;
}

void thread1() {
  int data2;
  data2 = get_data(1);
  assert (data2==10);
}

void move_data(int page, int new_location) {
  int loc = page_table[page];
  page_table[page] = new_location;
  memory[new_location] = memory[loc];
}

void thread2() {
  move_data(1,20);
}

void main() {
  page_table[1] = 5;
  memory[5] = 10;  
  
  thread1();
  thread2();
} 
