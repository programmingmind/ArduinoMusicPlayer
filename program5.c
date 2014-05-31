#include <avr/io.h>
#include <avr/interrupt.h>
#include "globals.h"
#include "os.h"
#include <util/delay.h>
#include <stdlib.h>
#include "ext2.h"
#include "SdReader.h"
#include "synchro.h"

uint16_t writeNdx, readNdx;
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
      buffer = (readNdx >> 8) & 1;

      mutex_lock(&mutexes[buffer]);
      
      getFileChunk(buffers[buffer]);
      readNdx += 256;
      
      mutex_unlock(&mutexes[buffer]);
   }
}

void printer() {
   uint8_t input;

   while (1) {
      while (byte_available()) {
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

   readNdx = 0;
   writeNdx = 256;

   //Create threads
   create_thread(writer, NULL, 16);
   create_thread(reader, NULL, 256);
   create_thread(printer, NULL, 32);

   os_start();
   sei();

   //Idle infinite loop
   while (1) {}
   
   return 0;
}
