#include "globals.h"

/*
 * Initialize the serial port.
 */
void serial_init() {
   uint16_t baud_setting;

   UCSR0A = _BV(U2X0);
   baud_setting = 16; //115200 baud

   // assign the baud_setting
   UBRR0H = baud_setting >> 8;
   UBRR0L = baud_setting;

   // enable transmit and receive
   UCSR0B |= (1 << TXEN0) | (1 << RXEN0);
}

/*
 * Return 1 if a character is available else return 0.
 */
uint8_t byte_available() {
   return (UCSR0A & (1 << RXC0)) ? 1 : 0;
}

/*
 * Unbuffered read
 * Return 255 if no character is available otherwise return available character.
 */
uint8_t read_byte() {
   if (UCSR0A & (1 << RXC0)) return UDR0;
   return 255;
}

/*
 * Unbuffered write
 *
 * b byte to write.
 */
uint8_t write_byte(uint8_t b) {
   //loop until the send buffer is empty
   while (((1 << UDRIE0) & UCSR0B) || !(UCSR0A & (1 << UDRE0))) {}

   //write out the byte
   UDR0 = b;
   return 1;
}

void write_int(uint32_t num) {
   uint8_t iVal[MAX_DIGITS], ndx = MAX_DIGITS;

   do {
      iVal[--ndx] = '0' + num % 10;
      num /= 10;
   } while (num > 0);

   while (ndx < MAX_DIGITS)
      write_byte(iVal[ndx++]);
}

void next_line() {
   //Restore cursor
   write_byte(ESC);
   write_byte('[');
   write_byte('u');
   //Move to next line
   write_byte(ESC);
   write_byte('[');
   write_byte('B');
}

static void save_cursor() {
   //Save cursor
   write_byte(ESC);
   write_byte('[');
   write_byte('s');
}

void print_string(char *s) {
   save_cursor();

   //Output String
   while (*s)
      write_byte(*s++);

}
void print_int(uint16_t i) {
   uint8_t iVal[5] = {0}, ndx = 5;

   //Store digits in array
   do {
      iVal[--ndx] = '0' + i % 10;
      i /= 10;
   } while (i > 0);
   //Print digits
   for (ndx = 0; ndx < 5; ndx++) {
      if (!iVal[ndx])
         write_byte(48);
      else
         write_byte(iVal[ndx]);
   }

}

void print_int32(uint32_t i) {
   uint8_t iVal[MAX_DIGITS] = {0}, ndx = MAX_DIGITS;

   //Store digits in array
   do {
      iVal[--ndx] = '0' + i % 10;
      i /= 10;
   } while (i > 0);
   //Print digits
   for (ndx = 0; ndx < MAX_DIGITS; ndx++) {
      if (!iVal[ndx])
         write_byte(48);
      else
         write_byte(iVal[ndx]);
   }
}

void print_hex(uint16_t i) {
   uint8_t iVal[4] = {0}, ndx = 4;
   uint16_t digit;

   //Store digits in array
   do {
      digit = i % 16;
      if (digit > 9)
         iVal[--ndx] = 'A' + digit - 10;
      else
         iVal[--ndx] = '0' + digit;
      i /= 16;
   } while (i > 0);
   //Print digits
   write_byte('0');
   write_byte('x');
   for (ndx = 0; ndx < 4; ndx++) {
      if (!iVal[ndx])
         write_byte(48);
      else
         write_byte(iVal[ndx]);
   }

}

void print_hex32(uint32_t i) {
   uint8_t iVal[MAX_DIGITS], ndx = MAX_DIGITS;
   uint16_t digit;

   //Store digits in array
   do {
      digit = i % 16;
      if (digit > 9)
         iVal[--ndx] = 'A' + digit - 10;
      else
         iVal[--ndx] = '0' + digit;
      i /= 16;
   } while (i > 0);
   //Print digits
   write_byte('0');
   write_byte('x');
   while (ndx < MAX_DIGITS)
      write_byte(iVal[ndx++]);

}

void set_cursor(uint8_t row, uint8_t col) {
   uint8_t iVal[MAX_DIGITS], ndx = MAX_DIGITS;

   write_byte(ESC);
   write_byte('[');
   //Generate sequence for row
   do {
      iVal[--ndx] = '0' + row % 10;
      row /= 10;
   } while (row > 0);
   //Print digits
   while (ndx < MAX_DIGITS)
      write_byte(iVal[ndx++]);

   write_byte(';');

   ndx = MAX_DIGITS;
   //Generate sequence for col
   do {
      iVal[--ndx] = '0' + col % 10;
      col /= 10;
   } while (col > 0);
   //Print digits
   while (ndx < MAX_DIGITS)
      write_byte(iVal[ndx++]);

   write_byte('H');

}

void clear_screen() {
   //Erase Screen
   write_byte(ESC);
   write_byte('[');
   write_byte('2');
   write_byte('J');
   //Move cursor to home
   write_byte(ESC);
   write_byte('[');
   write_byte('H');
}
