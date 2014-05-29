#include "os.h"
#include "globals.h"
#include "synchro.h"

void led_on() {
   //Set DDRB bit 5 to 1
   asm volatile ("LDI r30, 0x24");
   asm volatile ("LD r29, Z");
   asm volatile ("ORI r29, 0x20");
   asm volatile ("ST Z, r29");
   //Set PORTB bit 5 to 1
   asm volatile ("LDI r30, 0x25");
   asm volatile ("LD r29, Z");
   asm volatile ("ORI r29, 0x20");
   asm volatile ("ST Z, r29");
}

void led_off() {
   //Set DDRB bit 5 to 1
   asm volatile ("LDI r30, 0x24");
   asm volatile ("LD r29, Z");
   asm volatile ("ORI r29, 0x20");
   asm volatile ("ST Z, r29");
   //Set PORTB bit 5 to 0
   asm volatile ("LDI r30, 0x25");
   asm volatile ("LD r29, Z");
   asm volatile ("ANDI r29, 0xDF");
   asm volatile ("ST Z, r29");
}

void blink() {

   while (1) {

      if (full.value < BUFFER_SIZE)
         led_on();
      else
         led_off();
   }
}

void display_stats() {
   uint8_t i, input;

   while (1) {

      if (byte_available()) {
         input = read_byte();

         switch (input) {
         case 'a':
            produceRate += 100;
            break;
         case 'z':
            if (produceRate > 100)
               produceRate -= 100;
            break;
         case 'k':
            consumeRate += 100;
            break;
         case 'm':
            if (consumeRate > 100)
               consumeRate -= 100;
            break;
         default:
            break;
         }
      }
      mutex_lock(&m);

      set_cursor(0, 0);

      write_byte(ESC);
      write_byte('[');
      write_int(YELLOW);
      write_byte('m');

      print_string("System time (s): ");
      print_int(sysInfo.runtime);
      next_line();

      print_string("Interrupts/second: ");
      print_int(sysInfo.intrSec);
      next_line();

      print_string("Number of Threads: ");
      print_int(sysInfo.numThreads);
      next_line();

      for (i = 0; i < sysInfo.numThreads; i++) {
         set_cursor(5, i * 25);

         write_byte(ESC);
         write_byte('[');
         write_int(GREEN);
         write_byte('m');

         print_string("Thread id:    ");
         print_int(sysInfo.threads[i].id);
         next_line();

         print_string("Thread PC:    ");
         print_hex(sysInfo.threads[i].pc * 2);
         next_line();

         print_string("Stack usage:  ");
         if (i == 0)
            print_int(
             (uint16_t)sysInfo.threads[i].stackBase - sysInfo.threads[i].tp);
         else
            print_int(
             (uint16_t)sysInfo.threads[i].stackEnd - sysInfo.threads[i].tp);
         next_line();

         print_string("Stack size:   ");
         print_int(sysInfo.threads[i].totSize);
         next_line();

         print_string("Top of stack: ");
         print_hex(sysInfo.threads[i].tp);
         next_line();

         print_string("Stack base:   ");
         print_hex(sysInfo.threads[i].stackBase);
         next_line();

         print_string("Stack end:    ");
         print_hex(sysInfo.threads[i].stackEnd);
         next_line();

         print_string("Sched count:  ");
         print_int(sysInfo.threads[i].sched_count / sysInfo.runtime);
         next_line();

         print_string("PC interrupt: ");
         print_hex(((uint16_t)sysInfo.threads[i].intr_pcl
          + ((uint16_t)sysInfo.threads[i].intr_pch << 8)) * 2);
         next_line();
      }
      mutex_unlock(&m);
   }
}

void buffer_init() {

   memset(buffer, 0, BUFFER_SIZE);
   consumeRate = 1000;
   produceRate = 1000;
   in = 0;
   out = 0;
}

void display_bounded_buffer() {
   uint8_t i;
   while (1) {
      mutex_lock(&m);

      set_cursor(0, 30);
      write_byte(ESC);
      write_byte('[');
      write_int(RED);
      write_byte('m');

      print_string("Production rate (ms per item): ");
      print_int(produceRate);
      next_line();

      print_string("Consumption rate (ms per item): ");
      print_int(consumeRate);
      next_line();

      write_byte(124);
      for (i = 0; i < BUFFER_SIZE; i++) {
         if (buffer[i])
            write_byte(buffer[i]);
         else
            write_byte(32);
      }
      write_byte(124);

      mutex_unlock(&m);
   }
}

void producer() {
   uint8_t next_produced = 88;

   do {
      thread_sleep(produceRate/10);

      sem_wait(&empty);
      sem_wait(&mutex);

      buffer[in] = next_produced;

      sem_signal(&mutex);
      sem_signal(&full);

      in = (in + 1) % BUFFER_SIZE;

   } while (1);
}

void consumer() {
   uint8_t next_consumed;

   do {
      sem_wait(&full);
      sem_wait(&mutex);

      next_consumed = buffer[out];
      buffer[out] = 0;

      sem_signal(&mutex);
      sem_signal(&empty);

      out = (out + 1) % BUFFER_SIZE;
      thread_sleep(consumeRate/10);

   } while (1);
}

int main() {

   //Initialization
   serial_init();
   os_init();
   mutex_init(&m);
   sem_init(&s, 1);
   buffer_init();

   //Create threads
   create_thread(producer, NULL, 32);
   create_thread(consumer, NULL, 32);
   create_thread(display_stats, NULL, 32);
   create_thread(display_bounded_buffer, NULL, 32);
   create_thread(blink, NULL, 32);

   os_start();
   sei();

   //Idle infinite loop
   while (1) {}

   return 0;
}
