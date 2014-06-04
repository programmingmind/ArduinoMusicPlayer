#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "globals.h"
#include "os.h"
#include "ext2.h"
#include "SdReader.h"
#include "synchro.h"

uint8_t buffers[2][256];
mutex_t mutexes[2];

uint8_t numFiles, currentFile;

void writer() {
   uint8_t buffer = 0, pos = 0;

   while (1) {
      if (! pos) {
         mutex_unlock(&mutexes[buffer]);
         buffer ^= 1;
         mutex_lock(&mutexes[buffer]);
      }

      OCR2B = buffers[buffer][pos];
      pos++;

      thread_sleep(1);
   }
}

void reader() {
   uint8_t buffer = 0;

   while (1) {
      buffer ^= 1;

      mutex_lock(&mutexes[buffer]);
      
      getFileChunk(buffers[buffer]);
      
      mutex_unlock(&mutexes[buffer]);
   }
}

void printer() {
   uint8_t input, i;
   uint16_t curr, total;

   total = (getCurrentSize() >> 3) / 22100;

   while (1) {
      if (byte_available()) {
         input = read_byte();

         if (input == 'n') {
            currentFile = (currentFile + 1) % numFiles;
            getFile(currentFile);
            total = (getCurrentSize() >> 3) / 22100;
         } else if (input == 'p') {
            currentFile = currentFile ? currentFile - 1 : numFiles - 1;
            getFile(currentFile);
            total = (getCurrentSize() >> 3) / 22100;
         }
      }
      // show stats
      // name = getCurrentName();
      // length = (getCurrentSize() - WAV_HEADER) / 8;
      // currentTime = (getCurrentPos() - WAV_HEADER) / 8;

      set_color(YELLOW);

      set_cursor(1, 0);
      print_string("System time (s): ");
      print_int32(sysInfo.runtime);
      set_cursor(2, 0);
      print_string("Interrupts/second: ");
      print_int32(sysInfo.intrSec);
      set_cursor(3, 0);
      print_string("Number of Threads: ");
      print_int(sysInfo.numThreads);

      set_color(GREEN);

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

      set_cursor(12, 0);
      for (i = 0; i < NAME_LEN; i++)
         print_string(" ");

      set_cursor(12, 0);
      print_string(getCurrentName());

      curr = (getCurrentPos() >> 3) / 22100;

      set_cursor(13, 0);
      print_int(curr / 60);
      print_string(":");
      if (curr % 60 < 10)
         print_int(0);
      print_int(curr % 60);

      print_string(" / ");

      print_int(total / 60);
      print_string(":");
      if (total % 60 < 10)
         print_int(0);
      print_int(total % 60);

      print_string("  ");
   }
}

int main(void) {
   uint8_t sd_card_status;

   sd_card_status = sdInit(0);   //initialize the card with fast clock
                                 //if this does not work, try sdInit(1)
                                 //for a slower clock
   serial_init();
   ext2_init();

   numFiles = getNumFiles();
   currentFile = 0;
   getFile(currentFile);

   start_audio_pwm();
   os_init();

   mutex_init(&mutexes[0]);
   mutex_init(&mutexes[1]);

   uint8_t i = 255;
   while (i--)
      buffers[1][i] = 255 - (buffers[0][i] = i);

   //Create threads
   create_thread(writer, NULL, 32);
   // create_thread(reader, NULL, 256);
   create_thread(printer, NULL, 64);
   os_start();
   sei();

   //Idle infinite loop
   while (1) ;
   
   return 0;
}
