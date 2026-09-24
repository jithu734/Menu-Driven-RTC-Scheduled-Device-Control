#include<lpc21xx.h>  // Include header file for LPC21xx peripheral register definitions

#include<string.h>  // Include standard string manipulation functions (e.g., strcpy)

//cfg CLOCK FREQUENCY
#define FCLK 12000000        // Crystal oscillator frequency (12 MHz)
#define CCLK (5*FCLK)        // Core clock frequency via PLL (60 MHz)
#define PCLK  (CCLK/4)       // Peripheral clock frequency (15 MHz)

//RTC CLOCK FOR 1 TIC
#define PREINT_VAL ((PCLK/32768)-1)            // Integer prescaler value for Real Time Clock (RTC)
#define PREFRAC_VAL (PCLK-((PREINT+1)*32768))   // Fractional prescaler value for RTC

// LCD Command Definitions (HD44780 compatible)
#define FUN_SET 0x30     // Command to set 8-bit interface length
#define LCD_2L 0x38      // Command to configure LCD as 2 lines, 5x7 matrix, 8-bit mode
#define LCD_CLEAR 0x1    // Command to clear display and return cursor to home position
#define DISP_ON 0xC      // Command to turn display ON with cursor OFF
#define DISP_CUR_ON 0xE  // Command to turn display ON with solid cursor ON
#define DISP_CUR_BLINK_ON 0xF  // Command to turn display ON with blinking cursor
#define LINE_1 0x80      // DDRAM address for starting of Row 1
#define LINE_2 0xC0      // DDRAM address for starting of Row 2
#define CUR_SHIFT 0X6    // Command to set cursor entry mode (increment, no display shift)
#define CGRAM 0x40       // CGRAM start address for custom characters

// Hardware Pin Maps
#define LCD_PINS 8       // P0.8 - P0.15 used for LCD Data bus (D0-D7)
#define LCD_RS 17        // P0.17 connected to LCD Register Select (RS)
#define LCD_ENB 18       // P0.18 connected to LCD Enable pin (EN)

#define ROW_PINS 16      // P1.16 - P1.19 connected to Keypad Matrix Rows
#define COL_PINS 20      // P1.20 - P1.23 connected to Keypad Matrix Columns

#define ENT0_CHAN 14     // EINT0 (External Interrupt 0) VIC Channel Number

#define DEV0_PIN 30      // P1.30 connected to external output device/relay pin
//#define SHED_TIME(i,j)  (RTC_SHED_START[i]-'0')*10+(RTC_SHED_START[j]-'0')  // Macro to parse time ASCII digits into numerical values
#define GET_TIME_VAL(arr, i, j) (((arr)[i]-'0')*10 + ((arr)[j]-'0')) // Macro to parse time ASCII digits into numerical values
#define LPC2129          // Microcontroller model selection macro

// In-Application Programming (IAP) Definitions for Flash Memory operations
#define IAP_ADDR 0x7ffffff1    // Entry point memory address for LPC2000 IAP functions
#define Sector 7               // Target Flash sector for saving schedule data
#define Sector_Addr 0x00007000 // Flash Sector 7 base start address
#define CCLK_KHZ 60000         // Core Clock frequency expressed in kHz for IAP parameter
//#define LPC2129


typedef  char u8;          // 8-bit character data type
typedef unsigned int u32;  // 32-bit unsigned integer data type
typedef  float f32;        // 32-bit single-precision floating-point data type

// Function pointer declaration for calling In-Application Programming routines
typedef void (*IAP)(u32 [],u32 []);


/*----------------------FUNCTION PROTOTYPES---------------------*/
void INIT(void);
void rtc_init(void);
void LCD_INIT(void);
void LCD_CMD(u8 cmd);
void LCD_DETA(u8 data);
void LCD_STR(u8 *);
void LCD_INT(u32);
void LCD_FLOAT(f32);
void CGRAM_INIT(void);
char key_scan(void);
int col_scan(void);
int row_scan(void);
void INT_BUTTEN(void)__irq;  // Interrupt Service Routine for External Interrupt 0
void INT0_CONF(void);
void IO_DIR(void);
void delay_init(void);
//void delay_us(u32);
void delay_ms(u32);
void delay_ms1(u32 ms);
void Display(void);
void TIME(void);
void DAY(void);
void DATE(void);
void DisMoveUp(void);
void DisMoveDw(void);
void Edit_Time(void);
void Edit_Sehd(void);
void Time_set(u32 pos,u8 k_value);
void Update_Time(void);
void Update_Date(void);
void Edit_Shed_Time(u32 ,u8);
void Upload_shed(void);
void Update_Shed(void);
void Flage_call(void);
void calling(void);
void Show_Time_Menu(u32 idx);
void Show_Shed_Menu(u32 idx);
u32 Get_Num_Input(u32 digits, u32 minv, u32 maxv, u8 *ok);

/*----------------------GLOBAL VARIABLES---------------------*/
// Data buffer reserved for reading/writing Flash memory (4-byte aligned for IAP operations)
char Data_Buffer[512] __attribute__((aligned(4)));

// Strings storing default formatted strings for LCD output
u8 Time[]={"00:00:00        "};
u8 Date[]={"00/00/2026  LED"};

// 4x4 Keypad Keymap Matrix
u8 KPM[4][4]={"789%",
              "456*",
              "123-",
              "c0=+"};

// Menu navigation text strings
u8 MENU[3][15]={"1.EDIT-TIME",
                "2.E_dev_T_she",
                "3.EXIT"};

// Sub-menu for Edit Time: set individual fields with ranges (no SEC)
u8 TIME_MENU[7][16]={
                "1.SET HH 00-23",
                "2.SET MM 00-59",
                "3.SET DAY 0-6 ",
                "4.SET DOM01-31",
                "5.SET MON01-12",
                "6.SET YEAR    ",
                "7.EXIT        "};

// Sub-menu for Edit Schedule: set ON/OFF time fields with ranges (no SEC)
u8 SHED_MENU[5][16]={
                "1.ON HH 00-23 ",
                "2.ON MM 00-59 ",
                "3.OF HH 00-23 ",
                "4.OF MM 00-59 ",
                "5.EXIT        "};

// Schedules for device control: Start time and End time
u8 RTC_SHED_START[]={"ON:-00:00:00    "};
u8 RTC_SHED_END[]  ={"OF:-00:00:00    "};

u8 CGRAM_SPC[]={0x04,0x0E,0X1F,0X1F,0X04,0X04,0X04,0X00,0X04,0X04,0X04,0X1F,0X1F,0X0E,0X04,0X00,0x20,0x20,0x01,0x03,0x16,0x1c,0x08,0x00,0x20,0x11,0x0a,0x04,0x0a,0x11,0x20,0x00,0X20,0X04,0X0e,0X1F,0X0E,0X04,0X20,0X00 };

u32 flage;
/*----------------------MAIN FUNCTION---------------------*/
int main()
{
        INIT();          // Initialize Peripherals (GPIO, Timers, EINT0, LCD, RTC)
        DATE();          // Sync current RTC Date to string buffer
        Update_Shed();   // Load device schedule parameters into string buffers

        while(1)
        {
                Display(); // Main loop: continually display clock/schedules and monitor device control
                                        if(flage)
                                        Flage_call();
        }
}

/*----------------------ALL-DRIVERS INITIALIZATION---------------------*/
void INIT()
{
        IO_DIR();       // Configure General Purpose I/O directions
        delay_init();   // Reset and configure hardware Timer0 and Timer1
        INT0_CONF();    // Configure VIC and EINT0 interrupt line
        LCD_INIT();     // Initialize 16x2 character LCD
                                rtc_init();     // Initialize hardware RTC prescalers and start clock
        CGRAM_INIT();   // Initialize Lcd CGRAM characters
}


/*---------------------SET-IO_PINS_DIRECTIONS------------------------*/
void IO_DIR()
{
        // Set Port 0 LCD data pins (P0.8-P0.15), RS (P0.17), EN (P0.18) as outputs
        IODIR0|=0xff<<LCD_PINS | 1<<LCD_RS  | 1<<LCD_ENB;

        // Set Port 1 Keypad Row pins (P1.16-P1.19) and Device Pin (P1.30) as outputs
        IODIR1|=0xf<<ROW_PINS|1<<DEV0_PIN;
}

/*--------------------LCD_DISPLAY_DRIVER--------------------------------*/
void LCD_INIT()
{
        delay_ms(15);         // Power-on delay (>15ms)
        LCD_CMD(FUN_SET);     // Initialization sequence
        delay_ms(5);          // Delay (>4.1ms)
        LCD_CMD(FUN_SET);     // Second initialization call
        delay_ms(1);          // Delay (>100us)
        LCD_CMD(FUN_SET);     // Third initialization call
        LCD_CMD(LCD_2L);      // 8-bit mode, 2 lines, 5x7 dots
        LCD_CMD(LCD_CLEAR);   // Clear display
        LCD_CMD(CUR_SHIFT);   // Entry mode: Auto increment cursor
        LCD_CMD(DISP_ON);     // Display ON, Cursor OFF
}

// Send control command byte to LCD
void LCD_CMD(u8 cmd)
{
        IOCLR0=0xff<<LCD_PINS; // Clear LCD data lines
        IOSET0=cmd<<LCD_PINS;  // Load command on data lines
        IOCLR0=1<<LCD_RS;      // RS = 0 for Command Register selection
        IOSET0=1<<LCD_ENB;     // Pulse Enable Pin HIGH
        delay_ms(1);
        IOCLR0=1<<LCD_ENB;     // Pulse Enable Pin LOW

}

// Send data byte to LCD for printing
void LCD_DETA(u8 data)
{
        IOCLR0=0xff<<LCD_PINS; // Clear LCD data lines
        IOSET0=data<<LCD_PINS; // Load data character on data lines
        IOSET0=1<<LCD_RS;      // RS = 1 for Data Register selection
        IOSET0=1<<LCD_ENB;     // Pulse Enable Pin HIGH
        delay_ms(1);
        IOCLR0=1<<LCD_ENB;     // Pulse Enable Pin LOW
}
void CGRAM_INIT()
{
        u32 ch=0;
        u8 *p=CGRAM_SPC;
        LCD_CMD(CGRAM);
        while(ch++<5)
        {
        while(*p)
        {
                LCD_DETA(*p++);
        }
        LCD_DETA(DISP_ON);
        p++;
}
}

/*---------------------------------LCD_DATA WRAPPERS-------------------------------------------------*/
// Display null-terminated string on LCD
void LCD_STR(u8 *deta_str)
{
        while(*deta_str)
        {
                LCD_DETA(*deta_str);
                deta_str++;
        }
}

// Convert integer to string and display on LCD
void LCD_INT(u32 num)
{
        int pos=0;
        u8 itoa[20];
        while(num)
        {
                itoa[pos++]=num%10+'0'; // Extract digits
                num/=10;
        }
        itoa[pos]='\0';

        // Display inverted digits sequentially
        while(itoa[pos]!=itoa[0])
        {
                pos--;
                LCD_DETA(itoa[pos]);

        }
}

// Placeholder for displaying floating-point values
void LCD_FLOAT(f32 fnum)
{
}

/*---------------------------------RTC DRIVER------------------------------------------*/
void rtc_init()
{
        #ifdef LPC2129
        PREINT=PREINT_VAL;    // Set Integer Prescaler value
        PREFRAC=PREFRAC_VAL;  // Set Fractional Prescaler value
        CCR=1<<0;             // Enable Real Time Clock (CLKEN = 1)
        #else
        CCR|=1<<4;            // Enable external clock source
        CCR|=1<<0;            // Start RTC
        #endif
}

/*-------------------------DISPLAY & LOGIC ENGINE--------------------------------------------*/
void Display()
{
        TIME(); // Fetch system time from RTC registers into Time string array

        // At midnight (00:00:00) or if day is unitialized, update Day and Date string representations
        if((HOUR==0 && MIN==0 && SEC==0 )|| Time[10]==' ')
        {
                DAY();
                LCD_CMD(LINE_1);
                DATE();
        }

        // Multiplex screen display based on seconds count (5-second alternate view)
        if(SEC%10<=5)
        {
                // Show current Time (Line 1) and Date (Line 2)
                LCD_CMD(LINE_1);
                LCD_STR(Time);
                LCD_CMD(LINE_2);
                LCD_STR(Date);
                                                                if((IOPIN1>>DEV0_PIN)&1)
                                                                {
                                           LCD_DETA(2);
                                                                }
                                                                else
                                                                {
                                           LCD_DETA(3);
                                                                }
        }
        else
        {
                // Show Device ON Schedule (Line 1) and OFF Schedule (Line 2)
                LCD_CMD(LINE_1);
                LCD_STR(RTC_SHED_START);
                LCD_CMD(LINE_2);
                LCD_STR(RTC_SHED_END);

        }

        // Compare RTC time with scheduled START time -> Turn ON target Device at P1.30
      if( (GET_TIME_VAL(RTC_SHED_START,4,5) < GET_TIME_VAL(RTC_SHED_END,4,5)) ||
    (GET_TIME_VAL(RTC_SHED_START,4,5)==GET_TIME_VAL(RTC_SHED_END,4,5) && GET_TIME_VAL(RTC_SHED_START,7,8) <= GET_TIME_VAL(RTC_SHED_END,7,8)) )
{
        if( (HOUR > GET_TIME_VAL(RTC_SHED_START,4,5) || (HOUR==GET_TIME_VAL(RTC_SHED_START,4,5) && MIN > GET_TIME_VAL(RTC_SHED_START,7,8)) ||
             (HOUR==GET_TIME_VAL(RTC_SHED_START,4,5) && MIN==GET_TIME_VAL(RTC_SHED_START,7,8) && SEC>=GET_TIME_VAL(RTC_SHED_START,10,11))) &&
            (HOUR < GET_TIME_VAL(RTC_SHED_END,4,5) || (HOUR==GET_TIME_VAL(RTC_SHED_END,4,5) && MIN < GET_TIME_VAL(RTC_SHED_END,7,8)) ||
             (HOUR==GET_TIME_VAL(RTC_SHED_END,4,5) && MIN==GET_TIME_VAL(RTC_SHED_END,7,8) && SEC<GET_TIME_VAL(RTC_SHED_END,10,11))) )
                IOSET1=1<<DEV0_PIN;
        else
                IOCLR1=1<<DEV0_PIN;
}
else
{
        if( (HOUR > GET_TIME_VAL(RTC_SHED_START,4,5) || (HOUR==GET_TIME_VAL(RTC_SHED_START,4,5) && MIN > GET_TIME_VAL(RTC_SHED_START,7,8)) ||
             (HOUR==GET_TIME_VAL(RTC_SHED_START,4,5) && MIN==GET_TIME_VAL(RTC_SHED_START,7,8) && SEC>=GET_TIME_VAL(RTC_SHED_START,10,11))) ||
            (HOUR < GET_TIME_VAL(RTC_SHED_END,4,5) || (HOUR==GET_TIME_VAL(RTC_SHED_END,4,5) && MIN < GET_TIME_VAL(RTC_SHED_END,7,8)) ||
             (HOUR==GET_TIME_VAL(RTC_SHED_END,4,5) && MIN==GET_TIME_VAL(RTC_SHED_END,7,8) && SEC<GET_TIME_VAL(RTC_SHED_END,10,11))) )
                IOSET1=1<<DEV0_PIN;
        else
                IOCLR1=1<<DEV0_PIN;
}			
       

}

// Convert internal RTC registers (HOUR, MIN, SEC) into ASCII format
void TIME(void)
{
        Time[0]=HOUR/10+'0';
        Time[1]=HOUR%10+'0';
        Time[3]=MIN/10+'0';
        Time[4]=MIN%10+'0';
        Time[6]=SEC/10+'0';
        Time[7]=SEC%10+'0';
}

// Map Day Of Week register (DOW) to 3-letter day abbreviation
void DAY()
{
        switch(DOW)
        {
                case 0:Time[10]='S';Time[11]='U';Time[12]='N';break;// Sunday
                case 1:Time[10]='M';Time[11]='O';Time[12]='N';break;// Monday
                case 2:Time[10]='T';Time[11]='U';Time[12]='E';break;// Tuesday
                case 3:Time[10]='W';Time[11]='E';Time[12]='D';break;// Wednesday
                case 4:Time[10]='T';Time[11]='H';Time[12]='U';break;// Thursday
                case 5:Time[10]='F';Time[11]='R';Time[12]='I';break;// Friday
                case 6:Time[10]='S';Time[11]='A';Time[12]='T';break;// Saturday

        }
}

// Convert internal RTC date registers (DOM, MONTH, YEAR) into ASCII format
void DATE(void)
{
        int year;
        year=YEAR;
        Date[0]=DOM/10+'0';
        Date[1]=DOM%10+'0';
        Date[3]=MONTH/10+'0';
        Date[4]=MONTH%10+'0';
        Date[9]=year%10+'0';
        year/=10;
        Date[8]=year%10+'0';
        year/=10;
        Date[7]=year%10+'0';
        year/=10;
        Date[6]=year+'0';
}


/*-----------------------------EXTERNAL INTERRUPT 0 (EINT0) SETUP--------------------------------------------*/
void INT0_CONF()
{
        PINSEL1|=1<<0;             // Select P0.16 functional pin as EINT0
        VICIntSelect=0<<ENT0_CHAN; // Set EINT0 channel as IRQ type
        VICIntEnable=1<<ENT0_CHAN; // Enable VIC channel 14
        VICVectAddr0=(u32)INT_BUTTEN;// Assign address of ISR handler function
        VICVectCntl0=1<<5|ENT0_CHAN; // Enable vector slot 0 and bind channel 14
        EXTMODE|=1<<0;             // Configure External Interrupt 0 to be edge-sensitive
}

// Interrupt Service Routine triggered by pressing the key/button on EINT0
void INT_BUTTEN()__irq
{
    flage=1;
    VICVectAddr=0;           // Acknowledge interrupt to VIC logic
    EXTINT|=1<<ENT0_CHAN;    // Clear External Interrupt 0 flag
}
/*---------------------------flage_call-----------------------*/
void Flage_call()
{
        char k_valu;
        LCD_CMD(LCD_CLEAR);
        LCD_CMD(LINE_1);
        LCD_STR(MENU[0]);      // Display Menu Choice 1
        LCD_CMD(LINE_1+15);
        //LCD_STR("U.");
              LCD_DETA(0);
        LCD_CMD(LINE_2);
        LCD_STR(MENU[1]);      // Display Menu Choice 2
        LCD_CMD(LINE_2+15);
        //LCD_STR("D.");
              LCD_DETA(1);

        delay_ms1(10000);      // Setup Timer1 countdown window for menu selection

        // Stay in menu loop until Timer1 matches target timeout
        while(T1MR0!=T1TC)
        {

                        // Scan keypad when column inputs detect keypress low state
                        if(((IOPIN1>>COL_PINS)&15)!=15)
                        {
                        k_valu=key_scan();
                        switch(k_valu)

                        {
                                case '-':DisMoveUp();T1TC=0;break;  // Scroll Up in Menu (swapped)
                                case '+':DisMoveDw();T1TC=0;break;  // Scroll Down in Menu (swapped)

                                // Menu 1 selected: Modify current Date/Time values
                                case '1':Edit_Time();DisMoveDw();Update_Time();Update_Date();
                                         delay_ms1(5000);
                                         break;

                                // Menu 2 selected: Modify Device ON/OFF schedule
                                case '2':Edit_Sehd();T1TC=0;DisMoveDw();//Upload_shed();Update_Shed();
                                         delay_ms1(5000);break;

                                // Exit menu option
                                case '3':T1TC=T1MR0;

                        }


          }
        }
        flage=0;
}


/*-----------------------------TIMER DELAY DRIVERS--------------------------------------------*/
// Initialize hardware Timers (Timer 0 and Timer 1)
void delay_init()
{
        // Reset Timer Counters (Set Counter Reset bit HIGH)
        T0TCR=1<<1;
        T1TCR=1<<1;

        // Stop Timer on Match Match Register 0
        T0MCR=1<<2;
        T1MCR=1<<2;
}

// Millisecond hardware delay generator using Timer0
void delay_ms(u32 ms)
{
        T0MR0=ms;
        T0PR=15000-1;  // Prescaler configured for 1ms tick given 15MHz PCLK
        T0TC=0;        // Reset count
        T0TCR=1<<0;    // Start Timer0
        while(T0MR0!=T0TC); // Busy-wait until target count is reached
}

// Non-blocking timeout generator using Timer1
void delay_ms1(u32 ms)
{
        T1MR0=ms;
        T1PR=15000-1;  // Prescaler configured for 1ms tick
        T1TC=0;        // Reset count
        T1TCR=1<<0;    // Start Timer1
}

/*-----------------------KEYPAD MATRIX SCANNING DRIVERS-------------------------*/
// Main Keypad scanning function
char key_scan(void)
{
        int col,row;
        while(((IOPIN1>>COL_PINS)&15)!=15)
        {
        col=col_scan();      // Decode active column index
        row=row_scan();      // Decode active row index
        delay_ms(100);       // Debounce delay

        while(((IOPIN1>>COL_PINS)&15)!=15); // Wait until button is released
        return KPM[row][col];               // Return lookup character key
        }
        return 0;
}

// Scans active column pins P1.20 - P1.23
int col_scan(void)
{
        int inc=0;
        while(((IOPIN1>>(COL_PINS+inc))&1)==1)
        {
        inc++;
        }
        return inc;

}

// Drives row pins P1.16 - P1.19 sequentially to find active row
int row_scan(void)
{
        int inc=-1;
         while(((IOPIN1>>COL_PINS)&15)!=15)
         {
                inc++;
                IOPIN1=1<<(ROW_PINS+inc); // Drive single row HIGH
         }
         IOPIN1&=~(0xf<<ROW_PINS);        // Clear all row lines
         return inc;
}

/*-----------------------UI MENU & EDITING INTERFACES------------------------*/
// Shift menu screen view upward
void DisMoveUp(void)
{
        LCD_CMD(LCD_CLEAR);
        LCD_CMD(LINE_1);
        LCD_STR(MENU[1]);
       LCD_CMD(LINE_1+15);
        //LCD_STR("U.");
              LCD_DETA(0);
        LCD_CMD(LINE_2);
        LCD_STR(MENU[2]);
        LCD_CMD(LINE_2+15);
        //LCD_STR("D.");
              LCD_DETA(1);
}

// Shift menu screen view downward
void DisMoveDw(void)
{
        LCD_CMD(LCD_CLEAR);
        LCD_CMD(LINE_1);
        LCD_STR(MENU[0]);
     LCD_CMD(LINE_1+15);
        //LCD_STR("U.");
              LCD_DETA(0);
        LCD_CMD(LINE_2);
        LCD_STR(MENU[1]);
        LCD_CMD(LINE_2+15);
        //LCD_STR("D.");
              LCD_DETA(1);
}
// Show two consecutive TIME_MENU items (idx and idx+1)
void Show_Time_Menu(u32 idx)
{
        LCD_CMD(LCD_CLEAR);
        LCD_CMD(LINE_1);
        LCD_STR(TIME_MENU[idx]);
        LCD_CMD(LINE_1+13);
        LCD_STR("U.");
        LCD_DETA(0);
        if(idx+1 < 7)
        {
                LCD_CMD(LINE_2);
                LCD_STR(TIME_MENU[idx+1]);
                LCD_CMD(LINE_2+13);
                LCD_STR("D.");
                LCD_DETA(1);
        }
}

// Show two consecutive SHED_MENU items
void Show_Shed_Menu(u32 idx)
{
        LCD_CMD(LCD_CLEAR);
        LCD_CMD(LINE_1);
        LCD_STR(SHED_MENU[idx]);
        LCD_CMD(LINE_1+15);
        //LCD_STR("U.");
        LCD_DETA(0);
        if(idx+1 < 5)
        {
                LCD_CMD(LINE_2);
                LCD_STR(SHED_MENU[idx+1]);
                LCD_CMD(LINE_2+15);
                //LCD_STR("D.");
                LCD_DETA(1);
        }
}

// Read digits from keypad; accept only if final value is in [minv,maxv].
// On out-of-range or cancel: *ok=0 and value is discarded (no change).
u32 Get_Num_Input(u32 digits, u32 minv, u32 maxv, u8 *ok)
{
        u32 val=0, cnt=0;
        char k;
        *ok=0;
        LCD_CMD(LINE_2);
        LCD_STR("ENT:        ");
        LCD_CMD(LINE_2+4);
        while(T1MR0!=T1TC)
        {
                if(((IOPIN1>>COL_PINS)&15)!=15)
                {
                        k=key_scan();
                        if(k>='0' && k<='9' && cnt<digits)
                        {
                                val = val*10 + (k-'0');
                                LCD_DETA(k);
                                cnt++;
                                T1TC=0;
                        }
                        else if(k=='c')
                        {
                                *ok=0;
                                return 0;
                        }
                        else if(k=='=' || k=='+')  // confirm
                        {
                                if(cnt>0 && val>=minv && val<=maxv)
                                {
                                        *ok=1;
                                        return val;
                                }
                                // wrong range -> ignore, stay waiting
                                LCD_CMD(LINE_2);
                                LCD_STR("BAD RNG ");
                                delay_ms(800);
                                LCD_CMD(LINE_2);
                                LCD_STR("ENT:        ");
                                LCD_CMD(LINE_2+4);
                                val=0; cnt=0;
                                T1TC=0;
                        }
                }
        }
        *ok=0;
        return 0;
}

// Interface to edit System Time and Date via field-select menu (no SEC)
void Edit_Time(void)
{
        u32 midx=0, k_value, num;
        u8 ok;
        T1TC=0;
        delay_ms1(15000);   // menu timeout window
        Show_Time_Menu(midx);

        while(T1MR0!=T1TC)
        {
                if(((IOPIN1>>COL_PINS)&15)!=15)
                {
                        k_value=key_scan();
                        switch(k_value)
                        {
                                case '-': // scroll up  (swapped: - moves up)
                                        if(midx>0) midx--;
                                        Show_Time_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '+': // scroll down (swapped: + moves down)
                                        if(midx<5) midx++;
                                        Show_Time_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '1': // SET HH 00-23
                                        LCD_CMD(LCD_CLEAR);
                                        LCD_CMD(LINE_1);
                                        LCD_STR("HH 00-23:");
                                        num=Get_Num_Input(2,0,23,&ok);
                                        if(ok)
                                        {
                                                Time[0]=(num/10)+'0';
                                                Time[1]=(num%10)+'0';
                                        }
                                        Show_Time_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '2': // SET MM 00-59
                                        LCD_CMD(LCD_CLEAR);
                                        LCD_CMD(LINE_1);
                                        LCD_STR("MM 00-59:");
                                        num=Get_Num_Input(2,0,59,&ok);
                                        if(ok)
                                        {
                                                Time[3]=(num/10)+'0';
                                                Time[4]=(num%10)+'0';
                                        }
                                        Show_Time_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '3': // SET DAY 0-6
                                        LCD_CMD(LCD_CLEAR);
                                        LCD_CMD(LINE_1);
                                        LCD_STR("DAY 0-6:");
                                        num=Get_Num_Input(1,0,6,&ok);
                                        if(ok)
                                        {
                                                DOW=num;
                                                DAY();
                                        }
                                        Show_Time_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '4': // SET DOM 01-31
                                        LCD_CMD(LCD_CLEAR);
                                        LCD_CMD(LINE_1);
                                        LCD_STR("DOM 01-31:");
                                        num=Get_Num_Input(2,1,31,&ok);
                                        if(ok)
                                        {
                                                Date[0]=(num/10)+'0';
                                                Date[1]=(num%10)+'0';
                                        }
                                        Show_Time_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '5': // SET MON 01-12
                                        LCD_CMD(LCD_CLEAR);
                                        LCD_CMD(LINE_1);
                                        LCD_STR("MON 01-12:");
                                        num=Get_Num_Input(2,1,12,&ok);
                                        if(ok)
                                        {
                                                Date[3]=(num/10)+'0';
                                                Date[4]=(num%10)+'0';
                                        }
                                        Show_Time_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '6': // SET YEAR (4 digits, 2000-2099)
                                        LCD_CMD(LCD_CLEAR);
                                        LCD_CMD(LINE_1);
                                        LCD_STR("YEAR 2000-99:");
                                        num=Get_Num_Input(4,2000,2099,&ok);
                                        if(ok)
                                        {
                                                Date[6]=((num/1000)%10)+'0';
                                                Date[7]=((num/100)%10)+'0';
                                                Date[8]=((num/10)%10)+'0';
                                                Date[9]=(num%10)+'0';
                                        }
                                        Show_Time_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '7':
                                case 'c': // EXIT
                                        T1TC=T1MR0;
                                        break;
                        }
                }
        }
        LCD_CMD(DISP_ON);
}

// Interface to edit device Schedule (ON/OFF) via field-select menu (no SEC)
void Edit_Sehd(void)
{
        u32 midx=0, k_value, num;
        u8 ok;
        T1TC=0;
        delay_ms1(15000);
        Show_Shed_Menu(midx);

        while(T1MR0!=T1TC)
        {
                if(((IOPIN1>>COL_PINS)&15)!=15)
                {
                        k_value=key_scan();
                        switch(k_value)
                        {
                                case '-': // scroll up  (swapped)
                                        if(midx>0) midx--;
                                        Show_Shed_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '+': // scroll down (swapped)
                                        if(midx<3) midx++;
                                        Show_Shed_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '1': // ON HH 00-23
                                        LCD_CMD(LCD_CLEAR);
                                        LCD_CMD(LINE_1);
                                        LCD_STR("ON HH 00-23:");
                                        num=Get_Num_Input(2,0,23,&ok);
                                        if(ok)
                                        {
                                                RTC_SHED_START[4]=((num/10)+'0');
                                                RTC_SHED_START[5]=((num%10)+'0');
                                        }
                                        Show_Shed_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '2': // ON MM 00-59
                                        LCD_CMD(LCD_CLEAR);
                                        LCD_CMD(LINE_1);
                                        LCD_STR("ON MM 00-59:");
                                        num=Get_Num_Input(2,0,59,&ok);
                                        if(ok)
                                        {
                                                RTC_SHED_START[7]=((num/10)+'0');
                                                RTC_SHED_START[8]=((num%10)+'0');
                                        }
                                        Show_Shed_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '3': // OF HH 00-23
                                        LCD_CMD(LCD_CLEAR);
                                        LCD_CMD(LINE_1);
                                        LCD_STR("OF HH 00-23:");
                                        num=Get_Num_Input(2,0,23,&ok);
                                        if(ok)
                                        {
                                                RTC_SHED_END[4]=((num/10)+'0');
                                                RTC_SHED_END[5]=((num%10)+'0');
                                        }
                                        Show_Shed_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '4': // OF MM 00-59
                                        LCD_CMD(LCD_CLEAR);
                                        LCD_CMD(LINE_1);
                                        LCD_STR("OF MM 00-59:");
                                        num=Get_Num_Input(2,0,59,&ok);
                                        if(ok)
                                        {
                                                RTC_SHED_END[7]=((num/10)+'0');
                                                RTC_SHED_END[8]=((num%10)+'0');
                                        }
                                        Show_Shed_Menu(midx);
                                        T1TC=0;
                                        break;
                                case '5':
                                case 'c':
                                        T1TC=T1MR0;
                                        break;
                        }
                }
        }
        LCD_CMD(DISP_ON);
}

// Validates key inputs and updates the current string arrays for Time & Date
void Time_set(u32 pos,u8 k_value)
{

        if(pos==0||pos==1||pos==3||pos==4||pos==6||pos==7)
        {
                                                                // Validate lower digits of minutes/seconds (0-9)
                                                                if((pos==4 || pos==7)&&(k_value-'0')<=9)
                                                                {
                                Time[pos]=k_value;
                                                                }
                                                                // Validate tens digits of minutes/seconds (0-5)
                                                                if((pos==3 || pos==6)&&(k_value-'0')<=5)
                                                                {
                                Time[pos]=k_value;
                                                                }
                                                                // Validate tens digit of hours (0-2)
                                                                if(pos==0 &&(k_value-'0')<=2)
                                                                {
                                Time[pos]=k_value;
                                                                        if(Time[pos]==2)
                                                                                Time[pos+1]='0';
                                                                }
                                                                // Validate units digit of hours (up to 23 hours MAX)
                                                                if(pos==1)
                                                                {
                                                                        if((Time[pos-1]-'0')==2 && (k_value-'0')<=3)
                                                                        {
                      Time[pos]=k_value;
                                                                        }
                                                                        else if ((Time[3]-'0')!=2)
                                                                        {
                                                                                Time[pos]=k_value;
                                                                        }
                                                                }
                                                                // Redraw LCD contents
                                                                LCD_CMD(0x80);
                                                                LCD_STR(Time);
                                                                LCD_CMD(0xc0);
                                                                LCD_STR(Date);
                                                                LCD_CMD(0x80+pos);
        }
                                // Set Day of Week index (0-6)
                                else if(pos>=10 && pos<=12)
                                {

                                        DOW=(k_value-'0');
                                        DAY();
                                        LCD_CMD(0x80);
                                        LCD_STR(Time);
                                        LCD_CMD(0xc0);
                                        LCD_STR(Date);
                                        LCD_CMD(0x80+pos);

                                }
                                // Set Date fields: Day of Month, Month, and Year digits
        else if(pos==16||pos==17||pos==19||pos==20||pos==22||pos==23||pos==24||pos==25)
        {

                                                                // Update Year digits directly
                                                                if(pos==22||pos==23||pos==24||pos==25)
                                Date[pos-16]=k_value;

                                                                // Validate tens digit of Day of Month (0-3)
                                                                if(pos==16 && (k_value-'0')<=3)
                                                                {
                                                                        Date[pos-16]=k_value;
                                                                        if((Date[0]-'0')==3)
                                                                        Date[pos-15]=0+'0';
                                                                }
                                                                // Validate units digit of Day of Month
                                                                if(pos==17 )
                                                                {
                                                                        if((Date[0]-'0')==3&&(k_value-'0')<=1)
                                                                        Date[pos-16]=k_value;
                                                                        else if((Date[0]-'0')!=3)
                                                                        Date[pos-16]=k_value;
                                                                }
                                                                // Validate tens digit of Month (0-1)
                                                                if(pos==19 && (k_value-'0')<=1)
                                                                {
                                                                        Date[pos-16]=k_value;
                                                                if((Date[3]-'0')==1)
                                                                        Date[pos-15]=0+'0';
                                                          }
                                                                // Validate units digit of Month
                                                                if(pos==20)
                                                                {
                                                                        if((Date[3]-'0')==1&&(k_value-'0')<=2)
                                                                        Date[pos-16]=k_value;
                                                                        else if((Date[3]-'0')!=1)
                                                                        Date[pos-16]=k_value;
                                                                }
                                                                // Redraw LCD display
                                                                LCD_CMD(0x80);
                                                                LCD_STR(Time);
                                                                LCD_CMD(0xc0);
                                                                LCD_STR(Date);
                                             LCD_CMD(0xc0+(pos-16));


        }

}

// Push string time inputs into RTC internal registers
void Update_Time(void)
{

                        SEC=(Time[6]-'0')*10+(Time[7]-'0');
                        MIN=(Time[3]-'0')*10+(Time[4]-'0');
                        HOUR=(Time[0]-'0')*10+(Time[1]-'0');
}

// Push string date inputs into RTC internal registers
void Update_Date(void)
{
        DOM=GET_TIME_VAL(Date,0,1);//(Date[0]-'0')*10+(Date[1]-'0');
        MONTH = GET_TIME_VAL(Date, 3, 4);//(Date[3]-'0')*10+(Date[4]-'0');
        YEAR = GET_TIME_VAL(Date, 6, 7) * 100 + GET_TIME_VAL(Date, 8, 9);//((((Date[6]-'0')*10+(Date[7]-'0'))*10+(Date[8]-'0'))*10+(Date[9]-'0'));
}

// Validation logic for setting device schedule parameters (ON/OFF times)
void Edit_Shed_Time(u32 pos,u8 k_value)
{
        // Edit START schedule digits
        if(pos==4||pos==5||pos==7||pos==8||pos==11||pos==10)
        {
                                                                if((pos==8 || pos==11)&&(k_value-'0')<=9)
                                                                {
                                RTC_SHED_START[pos]=k_value;
                                                                }
                                                                if((pos==7 || pos==10)&&(k_value-'0')<=5)
                                                                {
                                RTC_SHED_START[pos]=k_value;
                                                                }
                                                                if(pos==4 &&(k_value-'0')<=2)
                                                                {
                                RTC_SHED_START[pos]=k_value;
                                                                        if(RTC_SHED_START[pos]=='2')
                                                                                RTC_SHED_START[pos+1]='0';
                                                                }
                                                                if(pos==5 )
                                                                {
                                                                        if(RTC_SHED_START[pos-1]=='2' &&(k_value-'0')<=3)
                                                                        {
                                    RTC_SHED_START[pos]=k_value;
                                                                        }
                                                                        else if(RTC_SHED_START[pos-1]!='2')
                                                                        {
                                                                                RTC_SHED_START[pos]=k_value;
                                                                        }
                                                                }
                                                                LCD_CMD(0x80);
                                                                LCD_STR(RTC_SHED_START);
                                                                LCD_CMD(0xc0);
                                                                LCD_STR(RTC_SHED_END);
                                                                LCD_CMD(0x80+pos);

        }
        // Edit END schedule digits
        else if(pos==21||pos==26||pos==27||pos==20||pos==23||pos==24)
        {

                                                                if((pos==27 || pos==24)&&(k_value-'0')<=9)
                                                                {
                                RTC_SHED_END[pos-16]=k_value;
                                                                }
                                                                if((pos==26 || pos==23)&&(k_value-'0')<=5)
                                                                {
                                RTC_SHED_END[pos-16]=k_value;
                                                                }
                                                                if(pos==20 &&(k_value-'0')<=2)
                                                                {
                                RTC_SHED_END[pos-16]=k_value;
                                                                        if(RTC_SHED_END[pos-16]=='2')
                                                                                RTC_SHED_END[pos-15]='0';
                                                                }
                                                                if(pos==21)
                                                                {
                                                                        if(RTC_SHED_END[pos-1-16]=='2' &&(k_value-'0')<=3)
                      RTC_SHED_END[pos-16]=k_value;
                                                                        else if(RTC_SHED_END[pos-1]!='2')
                                                                                 RTC_SHED_END[pos-16]=k_value;
                                                                }
                                                                LCD_CMD(0x80);
                                                                LCD_STR(RTC_SHED_START);
                                                                LCD_CMD(0xc0);
                                                                LCD_STR(RTC_SHED_END);
                                             LCD_CMD(0xc0+(pos-16));


        }
}

/*----------------------------------------------IN-APPLICATION PROGRAMMING (FLASH WRITE)----------------------------------------*/
// Saves scheduled times into the MCU's internal Flash Memory (Sector 7) non-volatile storage
void Upload_shed(void)
{
        u32 Command[5],Result[3];
        IAP call_iap=(IAP)IAP_ADDR; // Create callable pointer to ROM IAP function address

        // Pack ON and OFF Schedule strings into RAM Flash Buffer
        strcpy(Data_Buffer,RTC_SHED_START);
        strcpy(Data_Buffer+16,RTC_SHED_END);

        // Step 1: Prepare Sector 7 for Write/Erase command (IAP Command 50)
        Command[0]=50;
        Command[1]=Sector;
        Command[2]=Sector;
        call_iap(Command,Result);

        // Step 2: Erase Flash Sector 7 (IAP Command 52)
        Command[0]=52;
        Command[1]=Sector;
        Command[2]=Sector;
        Command[3]=CCLK_KHZ;
        __disable_irq();            // Disable interrupts during active Flash write/erase operation
        call_iap(Command,Result);
        __enable_irq();             // Re-enable interrupts

        // Step 3: Prepare Sector 7 again for Copying RAM to Flash (IAP Command 50)
        Command[0]=50;
        Command[1]=Sector;
        Command[2]=Sector;
        call_iap(Command,Result);

        // Step 4: Copy RAM Buffer to Flash Sector Address (IAP Command 51)
        Command[0]=51;
        Command[1]=Sector_Addr;
        Command[2]=(u32)Data_Buffer;
        Command[3]=512;             // Write size = 512 bytes
        Command[4]=CCLK_KHZ;
        __disable_irq();
        call_iap(Command,Result);
        __enable_irq();
    if(Result[0]!=0)
    {
       calling();
    }
}

// Copy values stored in Memory to active working schedule strings
void Update_Shed()
{
        // Cast Flash Memory Sector 7 base address pointer to byte-accessible unsigned char pointer
        u8 *Data=(u8 *)Sector_Addr;

        // Read saved START time digits directly from Sector 7 Flash memory into the RTC_SHED_START array
        RTC_SHED_START[4]=Data[4];    // Hours tens digit   (e.g., '2' in "20")
        RTC_SHED_START[5]=Data[5];    // Hours units digit  (e.g., '0' in "20")
        RTC_SHED_START[7]=Data[7];    // Minutes tens digit (e.g., '3' in "35")
        RTC_SHED_START[8]=Data[8];    // Minutes units digit(e.g., '5' in "35")
        //RTC_SHED_START[11]=Data[11];  // Seconds units digit(e.g., '0' in "00")
       // RTC_SHED_START[10]=Data[10];  // Seconds tens digit (e.g., '0' in "00")

        // Read saved END time digits directly from offset Sector 7 Flash memory into the RTC_SHED_END array
        RTC_SHED_END[4]=(Data+16)[4];   // Hours tens digit
        RTC_SHED_END[5]=(Data+16)[5];   // Hours units digit
        RTC_SHED_END[7]=(Data+16)[7];   // Minutes tens digit
        RTC_SHED_END[8]=(Data+16)[8];   // Minutes units digit
       // RTC_SHED_END[10]=(Data+16)[10]; // Seconds tens digit
       // RTC_SHED_END[11]=(Data+16)[11]; // Seconds units digit
}
void calling()
{
LCD_DETA(LCD_CLEAR);
delay_ms(100000);
}
