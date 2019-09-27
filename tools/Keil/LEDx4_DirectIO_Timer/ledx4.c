#include <reg52.h>
#include "7seg.h"

// for STC89C52RC

// upside l->r
sbit k1 = P1^5;//1 12
sbit SA = P1^4;//a 11
sbit SF = P1^3;//f 10
sbit k2 = P1^2;//2 9
sbit k3 = P1^1;//3 8
sbit SB = P1^0;//b 7

// downside l->r
sbit SE = P2^0;//e 1
sbit SD = P2^1;//d 2
sbit SZ = P2^2;//dp 3
sbit SC = P2^3;//c 4
sbit SG = P2^4;//g 5
sbit k4 = P2^5;//4 6

unsigned int num;
unsigned int seconds = 9753;
unsigned int temp, d1, d2, d3, d4;

void turn_off(){
    k1 = k2 = k3 = k4 = 1;
}

void T0_timer() interrupt 1
{
    TH0 = (65536 - 45872) / 256;
    TL0 = (65536 - 45872) % 256;
    // 10ms * 100 = 1sec
    num++;
    if (num == 100) {
        num = 0;
        seconds++;
    }
}

void show_num(unsigned char num){
  unsigned char c = CATHODE[num];
  SA = (c >> 0) & 0x01;
  SB = (c >> 1) & 0x01;
  SC = (c >> 2) & 0x01;
  SD = (c >> 3) & 0x01;
  SE = (c >> 4) & 0x01;
  SF = (c >> 5) & 0x01;
  SG = (c >> 6) & 0x01;
  SZ = (c >> 7) & 0x01;

}

void delay(unsigned int z)
{
  unsigned int i,j;
  for(i=z;i>0;i--)
    for (j = 110; j > 0; j--)
      ;
}

void update_led()
{
    temp = seconds;
    if (temp > 9999) {
        temp = temp % 1000;
    }
    d1 = temp / 1000;
    temp = temp % 1000;
    d2 = temp / 100;
    temp = temp % 100;
    d3 = temp / 10;
    temp = temp % 10;
    d4 = temp;
    turn_off();
    k1 = 0;
    show_num(d1);
    delay(5);
    turn_off();
    k2 = 0;
    show_num(d2);
    delay(5);
    turn_off();
    k3 = 0;
    show_num(d3);
    delay(5);
    turn_off();
    k4 = 0;
    show_num(d4);
    delay(5);
}

void main()
{
    turn_off();
    TMOD = 0;
    // 11.0592MHz - 45872
    TH0 = (65536 - 45872) / 256;
    TL0 = (65536 - 45872) % 256;
    EA = 1;
    ET0 = 1;
    TR0 = 1;
    while (1) {
        update_led();
    }
}















