#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "globals.h"
#include "os.h"
#include "ext2.h"
#include "SdReader.h"
#include "synchro.h"

uint16_t writeNdx;
uint8_t readNdx;
uint8_t buffers[2][256];
mutex_t mutexes[2];

uint8_t numFiles, currentFile;

void writer() {
   uint8_t buffer, pos;

   while (1) {
      buffer = (writeNdx>>8) & 1;
      pos = writeNdx & 255;

      if (! pos) {
         mutex_unlock(&mutexes[1 - buffer]);
         mutex_lock(&mutexes[buffer]);
      }

      OCR2B = buffers[buffer][pos];
      writeNdx++;

      thread_sleep(1);
   }
}

void reader() {
   uint8_t buffer;

   while (1) {
      buffer = readNdx++ & 1;

      mutex_lock(&mutexes[buffer]);
      
      getFileChunk(buffers[buffer]);
      
      mutex_unlock(&mutexes[buffer]);
   }
}

void printer() {
   uint8_t input, i;

   while (1) {
      thread_sleep(20);

      mutex_lock(&mutexes[0]);

      if (byte_available()) {
         input = read_byte();

         if (input == 'n') {
            currentFile = (currentFile + 1) % numFiles;
            getFile(currentFile);
         } else if (input == 'p') {
            currentFile = currentFile ? currentFile - 1 : numFiles - 1;
            getFile(currentFile);
         }
      }
      // show stats
      // name = getCurrentName();
      // length = (getCurrentSize() - WAV_HEADER) / 8;
      // currentTime = (getCurrentPos() - WAV_HEADER) / 8;

      write_byte(ESC);
      write_byte('[');
      write_byte(YELLOW);
      write_byte('m');

      set_cursor(1, 0);
      print_string("System time (s): ");
      print_int32(sysInfo.runtime);
      set_cursor(2, 0);
      print_string("Interrupts/second: ");
      print_int32(sysInfo.intrSec);
      set_cursor(3, 0);
      print_string("Number of Threads: ");
      print_int(sysInfo.numThreads);

      write_byte(ESC);
      write_byte('[');
      write_byte(GREEN);
      write_byte('m');

      for (i = 0; i < sysInfo.numThreads; i++) {
         set_cursor(5, i * 25);
         print_string("Thread id:    ");
         print_int(sysInfo.threads[i].id);
         set_cursor(6, i * 25);
         print_string("Thread PC:    ");
         print_hex(sysInfo.threads[i].pc * 2);
         set_cursor(7, i * 25);
         print_string("Stack usage:  ");
         if (i == 0)
            print_int(
             (uint16_t)sysInfo.threads[i].stackBase - sysInfo.threads[i].tp);
         else
            print_int(
             (uint16_t)sysInfo.threads[i].stackEnd - sysInfo.threads[i].tp);
         set_cursor(8, i * 25);
         print_string("Stack size:   ");
         print_int(sysInfo.threads[i].totSize);
         // set_cursor(9, i * 25);
         // print_string("Top of stack: ");
         // print_hex(sysInfo.threads[i].tp);
         // set_cursor(10, i * 25);
         // print_string("Stack base:   ");
         // print_hex(sysInfo.threads[i].stackBase);
         // set_cursor(11, i * 25);
         // print_string("Stack end:    ");
         // print_hex(sysInfo.threads[i].stackEnd);
         // set_cursor(12, i * 25);
         // print_string("Sched count:  ");
         // print_int(sysInfo.threads[i].sched_count / sysInfo.runtime);
         // set_cursor(13, i * 25);
         // print_string("PC interrupt: ");
         // print_hex(((uint16_t)sysInfo.threads[i].intr_pcl
         //  + ((uint16_t)sysInfo.threads[i].intr_pch << 8)) * 2);
      }
      mutex_unlock(&mutexes[0]);
   }
}

void print2() {
   uint16_t test = 0;
   while (1) {
      mutex_lock(&mutexes[0]);
      set_cursor(10, 0);
      print_int32(test++);
      mutex_unlock(&mutexes[0]);
      thread_sleep(200);
   }
}

int main(void) {
   uint8_t sd_card_status;

   sd_card_status = sdInit(0);   //initialize the card with fast clock
   //                               //if this does not work, try sdInit(1)
   //                               //for a slower clock
   serial_init();
   ext2_init();

   // numFiles = getNumFiles();
   // currentFile = 0;
   // getFile(currentFile);

   start_audio_pwm();
   os_init();

   mutex_init(&mutexes[0]);
   mutex_init(&mutexes[1]);

   readNdx = 0;
   writeNdx = 0;

   //Create threads
   // create_thread(writer, NULL, 16);
   // create_thread(reader, NULL, 256);
   create_thread(printer, NULL, 32);
   create_thread(print2, NULL, 32);
   os_start();
   sei();

   uint32_t test = 0;
   //Idle infinite loop
   while (1) ;
   
   return 0;
}
