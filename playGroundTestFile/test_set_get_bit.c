#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

unsigned int get_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   return (bitmap[index] >> offset) & 0x1;
}

void set_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   bitmap[index] |= 0x1 << offset;
   //bitmap[index] &= ~(0x1 << offset);
}

int main(){

   unsigned char bitmap[20];

   // Initial bitmap setup
   bitmap[1] = 1;

   //////////////////////////

   printf("INITIAL STATE OF BITMAP:\n");
   for(int i = 0; i< 20; i++){
      printf("%d", get_bit((unsigned int*)bitmap, i));
   }
   printf("\n\n");

   printf("STATE OF BITMAP AFTER UPDATE:\n");
   // Update bitmap

   set_bit((unsigned int*)bitmap, 5);

   //////////////////////////
   for(int i = 0; i< 20; i++){
      printf("%d", get_bit((unsigned int*)bitmap, i));
   }
   printf("\n");

}