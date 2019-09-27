#include	"tm1638.h"
#include    <intrins.h>

unsigned char num[8];		//�����������ʾ��ֵ

void init()
{
	unsigned char i;
	init_TM1638();	                           //��ʼ��TM1638
	for(i=0;i<8;i++)
	Write_DATA(i<<1,tab[i]);	               //��ʼ���Ĵ���
}

void delay(int ms)
{
    int i, j;
    for(i = 0;i<ms;i++)
        for(j=0;j<100;j++)
            ;
}

void test_all()
{
	unsigned char i;
	while(1)
	{
		i=Read_key();                          //������ֵ
		if(i<8)
		{
			num[i]++;
			while(Read_key()==i);		       //�ȴ������ͷ�
			if(num[i]>15)
			num[i]=0;
			Write_DATA(i*2,tab[num[i]]);
			Write_allLED(1<<i);
		}
	}
}

// ��ˮ�ƣ��õ�8λ����ܣ�8��LED
// ͬʱ�������ʾ������λ��ֵ
void test_flash_led()
{
    unsigned char i, j;
    unsigned char v;
    v = 0xfe;//1111 1110
    while(1)
    {
        for(i=0;i<8;i++)
        {  
            for(j=0;j<8;j++)
                Write_DATA(j<<1, tab[(v & (1 << j))>>j]);
            Write_allLED(v);
            v = _crol_(v,1);
            //v =(v << 1) | (v >> 7);
            //v = _cror_(v,1);
            //v = (v >> 1) | (v << 7)
            delay(500);
        }
    }
}

int main(void)
{
    init();
    test_flash_led();
}

