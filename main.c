#include <reg51.h>
#include <intrins.h>

#define uchar unsigned char
#define uint unsigned int

sbit wela = P2^6;
sbit dula = P2^7;
sbit DSCLK = P1^0;
sbit DSIO = P1^1;
sbit DSRST = P1^2;

sbit temperature=P2^2; 
sbit lcdrs = P3^5;
sbit lcdrw = P3^6;
sbit lcden = P3^4;

sbit key = P3^7;

sbit iicscl = P2^0;
sbit iicsda = P2^1;

uchar code READ_RTC_ADDR[7] = {0x81, 0x83, 0x85, 0x87, 0x89, 0x8B, 0x8D};
uchar code WRITE_RTC_ADDR[7] = {0x80, 0x82, 0x84, 0x86, 0x88, 0x8A, 0x8C};
uchar TIME[7] = {0, 0x42, 0x17, 0x13, 0x04, 0x01, 0x15};
uchar code hello[]="hello";//5

uint tmpvl;

uchar timer;

uchar address;

void delay(uint z);
void DS1302Write(uchar addr, uchar dat);
uchar DS1302Read(uchar addr);
void DS1302Init();
void DS1302ReadTime();
void ds18b20_init(void);
void ds18b20_wait(void);
bit ds18b20_read_bit(void);
unsigned char ds18b20_read_byte(void);
void ds18b20_write_byte(unsigned char dat);
void ds18b20_send_change_command(void);
void ds18b20_send_read_command(void);
int ds18b20_get_temperature(void);
void iic_start();
void iic_stop();
void iic_respons();
void iic_no_respons();
void iic_init();
void iic_write_byte(uchar dat);
uchar iic_read_byte();
uchar iic_equipment_read_data(uchar add,uchar command);
void iic_equipment_write_data(uchar add, uchar command1,uchar command2);
void timer_init();
void write_com(uchar com);
void write_data(uchar date);
void init();
void LCD_Display_Time();
void LCD_Display_Temperature(uchar a, uchar b);

void main(){
//	uchar i;
	uint rdvl;
	uchar count = 0;
	init();
	iic_init();
	ds18b20_send_change_command();
	timer_init();
//	DS1302Init();
	while(1){
		tmpvl=ds18b20_get_temperature();
		DS1302ReadTime();
		LCD_Display_Time();
		LCD_Display_Temperature(tmpvl%100/10,tmpvl%10);
		ds18b20_send_change_command();
		if(key == 0)
		{
			delay(15);
			if(key == 0)
			{
				rdvl = iic_equipment_read_data(0xa1,count);
				write_com(0x8D);
				write_data(0x30+rdvl%100/10);
				write_com(0x8E);
				write_data(0x30+rdvl%10);
				count++;
				if(count == 20)
					count = 0;
			}
			while(!key);
			delay(15);
			while(!key);
		}
	}
}

void timer0_interrupt() interrupt 1
{
	TR0 = 0;
	ET0 = 0;
	TH1 = (65536-50000)/256;
	TL0 = (65536-50000)%256;
	timer++;
	if(timer == 100)
	{
		timer = 0;
		iic_equipment_write_data(0xa0,address,tmpvl);
		address++;
		if(address == 20){
			address = 0;
		}
	}
	TR0 = 1;
	ET0 = 1;
}

void delay(uint z){
	uint x,y;
	for(x=z;x>0;x--){
		for(y=110;y>0;y--);
	}
}


void DS1302Write(uchar addr, uchar dat){
	uchar i;
	DSRST = 0;
	_nop_();
	DSCLK = 0;
	_nop_();
	DSRST = 1;
	_nop_();
	
	for (i = 0; i < 8; i++){
		DSIO = addr & 0x01;
		addr >>= 1;
		DSCLK = 1;
		_nop_();
		DSCLK = 0;
		_nop_();
	}
	
	for (i = 0; i < 8; i++){
		DSIO = dat & 0x01;
		dat >>= 1;
		DSCLK = 1;
		_nop_();
		DSCLK = 0;
		_nop_();	
	}	
		 
	DSRST = 0;
	_nop_();
}

uchar DS1302Read(uchar addr){
	uchar n,dat,dat1;
	DSRST = 0;
	_nop_();

	DSCLK = 0;//先将SCLK置低电平。
	_nop_();
	DSRST = 1;//然后将RST(CE)置高电平。
	_nop_();

	for(n=0; n<8; n++){//开始传送八位地址命令
		DSIO = addr & 0x01;//数据从低位开始传送
		addr >>= 1;
		DSCLK = 1;//数据在上升沿时，DS1302读取数据
		_nop_();
		DSCLK = 0;//DS1302下降沿时，放置数据
		_nop_();
	}
	_nop_();
	for(n=0; n<8; n++){//读取8位数据
		dat1 = DSIO;//从最低位开始接收
		dat = (dat>>1) | (dat1<<7);
		DSCLK = 1;
		_nop_();
		DSCLK = 0;//DS1302下降沿时，放置数据
		_nop_();
	}

	DSRST = 0;
	_nop_();	//以下为DS1302复位的稳定时间,必须的。
	DSCLK = 1;
	_nop_();
	DSIO = 0;
	_nop_();
	DSIO = 1;
	_nop_();
	return dat;	
}

void DS1302Init(){
	uchar n;
	DS1302Write(0x8E,0X00);
	for(n=0; n<7; n++){
		DS1302Write(WRITE_RTC_ADDR[n],TIME[n]);	
	}
	DS1302Write(0x8E,0x80);
}

void DS1302ReadTime(){
	unsigned char n;
	for (n=0; n<7; n++)//读取7个字节的时钟信号：分秒时日月周年
	{
		TIME[n] = DS1302Read(READ_RTC_ADDR[n]);
	}
}

void ds18b20_init(void){
	unsigned int i;
	temperature=0;
	i=100;
	while(i>0)
		i--;
	temperature=1;
	i=4;
	while(i>0)
		i--;
}
//DS18B20等待函数
void ds18b20_wait(void){
	unsigned int i;
	while(temperature);
	while(~temperature);//IO口电平取反
	i=4;
	while(i>0)
		i--;
}
//DS18B20读取一位数据
bit ds18b20_read_bit(void){
	unsigned int i;
	bit b;
	temperature=0;
	i++;
	temperature=1;
	i++;
	i++;
	b=temperature;
	i=8;
	while(i>0)
		i--;
	return b;
}
//DS18B20读取数据
unsigned char ds18b20_read_byte(void){
	unsigned int i;
	unsigned char j,dat;
	dat=0;
	for(i=0;i<8;i++){
		j=ds18b20_read_bit();
		dat=(j<<7)|(dat>>1);
	}
	return dat;
}
//DS18B20写一位函数
void ds18b20_write_byte(unsigned char dat){
	unsigned int i;
	unsigned char j;
	bit b;
	for(j=0;j<8;j++){
		b=dat&0x01;
		dat>>=1;
		if(b){
			temperature=0;
			temperature=1;
			i=8;
			while(i>0)
				i--;
		}
		else{
			temperature=0;
			i=8;
			while(i>0)
				i--;
			temperature=1;
			i++;
			i++;
		}
	}
}
//DS18B20发送变换命令
void ds18b20_send_change_command(void){
	ds18b20_init();
	ds18b20_wait();
	delay(1);
	ds18b20_write_byte(0xcc);
	ds18b20_write_byte(0x44);
}
//DS18B20发送读取命令
void ds18b20_send_read_command(void){
	ds18b20_init();
	ds18b20_wait();
	delay(1);
	ds18b20_write_byte(0xcc);
	ds18b20_write_byte(0xbe);
}
//DS18B20读取温度
int ds18b20_get_temperature(void){
	unsigned int tmpvalue;
	int value;
	float t;
	unsigned char low,high;
	ds18b20_send_read_command();
	low=ds18b20_read_byte();
	high=ds18b20_read_byte();
	tmpvalue=high;
	tmpvalue<<=8;
	tmpvalue|=low;
	value=tmpvalue;
	t=value*0.0625;
	value=t+(value>0?0.5:-0.5);//判断VALUE的值，如果大于0，则加0.5否则-0.5
	return value;
}

void iic_start()
{
	iicsda=1;
	_nop_();
	iicscl=1;
	_nop_();
	iicsda=0;
	_nop_();
}

void iic_stop()
{
	iicsda=0;
	_nop_();
	iicscl=1;
	_nop_();
	iicsda=1;
	_nop_();
}

void iic_respons()
{
	uchar i;
	iicscl=1;
	_nop_();
	while((iicsda==1)&&(i<255))
		i++;
	iicscl=0;
	_nop_();
}

void iic_no_respons()
{
	iicsda=1;
	_nop_();
	iicscl=1;
	_nop_();
	iicscl=0;
	_nop_();
}

void iic_init()
{
	iicsda=1;
	iicscl=1;
}

void iic_write_byte(uchar dat)
{
	uchar i;
	iicscl=0;
	for(i=0;i<8;i++)
	{
		if (dat & 0x80)
		{
			iicsda=1;
		}
		else
		{
			iicsda=0;
		}
		dat=dat<<1;
		_nop_();
		iicscl=1;
		_nop_();
		iicscl=0;
		_nop_();
	}
	iicsda=1;
	_nop_();
}

uchar iic_read_byte()
{
	uchar i,dat;
	iicscl=0;
	_nop_();
	iicsda=1;
	_nop_();
	for(i=0;i<8;i++)
	{
		iicscl=1;
		_nop_();
		dat<<=1;
		if (iicsda)
		{
			dat++;
		}
		iicscl=0;
		_nop_();
	}
	return dat;
}

uchar iic_equipment_read_data(uchar add,uchar command)
{
	uchar temp;
	uchar tt=add&0xfe;
	iic_init();
	iic_start();
	iic_write_byte(tt);
	iic_respons();
	iic_write_byte(command);
	iic_respons();
	iic_start();
	iic_write_byte(add);
	iic_respons();
	temp=iic_read_byte();
	iic_no_respons();
	iic_stop();
	return temp;
}

void iic_equipment_write_data(uchar add, uchar command1,uchar command2)
{
	iic_init();
	iic_start();
	iic_write_byte(add);
	iic_respons();
	iic_write_byte(command1);
	iic_respons();
	iic_write_byte(command2);
	iic_respons();
	iic_stop();
}

void timer_init(){
	TMOD = 0X01;
	TH1 = (65536-50000)/256;
	TL0 = (65536-50000)%256;
	ET0 = 1;
	TR0 = 1;
	EA = 1;
}
void write_com(uchar com)
{
	lcdrs=0;
	P0=com;
	delay(5);
	lcden=1;
	delay(5);
	lcden=0;
}

void write_data(uchar date)
{
	lcdrs=1;
	P0=date;
	delay(5);
	lcden=1;
	delay(5);
	lcden=0;
}

void init()
{
	timer = 0;
	wela = 0;
	dula = 0;
	lcdrw = 0;
	lcden=0;
	write_com(0x38);
	write_com(0x0e);
	write_com(0x06);
	write_com(0x01);
	write_com(0x80+0x10);
}

void LCD_Display_Time(){
	write_com(0x80);
	write_data('2');
	write_com(0x81);
	write_data('0');
	write_com(0x82);
	write_data(0x30+TIME[6]/16);
	write_com(0x83);
	write_data(0x30+TIME[6]%16);
	write_com(0x84);
	write_data('-');
	write_com(0x85);
	write_data(0x30+TIME[4]/16);
	write_com(0x86);
	write_data(0x30+TIME[4]%16);
	write_com(0x87);
	write_data('-');
	write_com(0x88);
	write_data(0x30+TIME[3]/16);
	write_com(0x89);
	write_data(0x30+TIME[3]%16);
	write_com(0x8A);
	write_data(' ');
	write_com(0x8B);
	write_data(0x30+TIME[5]%16);
	write_com(0x80+0X40);
	write_data(0x30+TIME[2]/16);
	write_com(0x80+0X41);
	write_data(0x30+TIME[2]%16);
	write_com(0x80+0X42);
	write_data(':');
	write_com(0x80+0X43);
	write_data(0x30+TIME[1]/16);
	write_com(0x80+0X44);
	write_data(0x30+TIME[1]%16);
	write_com(0x80+0X45);
	write_data(':');
	write_com(0x80+0X46);
	write_data(0x30+TIME[0]/16);
	write_com(0x80+0X47);
	write_data(0x30+TIME[0]%16);
}

void LCD_Display_Temperature(uchar a, uchar b){
	write_com(0x80+0X49);
	write_data(0x30+a);
	write_com(0x80+0X4A);
	write_data(0x30+b);
	write_com(0x80+0X4B);
	write_data('`');
	write_com(0x80+0X4C);
	write_data('C');
}