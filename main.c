/*This project includes the GOL,MLA libraries provided by Microchip website and uses PIC24EP Mikromedia Board.*/
#include "PICconfig.h"
#include "HardwareProfile.h"
#include "uMedia.h"
#include "LCDTerminal.h"
#include "TouchScreen.h"
#include <Graphics/GOL.h>
#include <TouchScreen.h>
#include <M25P80.h>
#include "GraphicsConfig.h"
#include <stdio.h>
#include "xc.h"
#include "Graphics/Graphics.h"
#include <p24ep512gu810.h>
#include <stdlib.h>
#include <string.h>
                 
#define __ISR __attribute__((interrupt, shadow, no_auto_psv))
#define TICK_PERIOD( ms) (GetPeripheralClock()*(ms)) /8000
#define PASSWORD_LEN    4  // Password length

void CreatePinKeypad();
void RTCInit();
void Alarm();
void User_SetAlarm();
void User_ResetAlarm();

void AlarmVal_set();
void  RTCC_ALM_Reset();
void Temperature();
void MyGOLDraw();
void initADC(void);
void CreateButtons();
void SetCommandKeys(TEXTENTRY *pTe);
void CreatePinKeypad();
void CreateNumPinKeypad();
void main_page();

static int alarm_index=0;
static int index=0;
static pinapp_active=0;
static alarmapp_active=0;
GOL_MSG msg;
int  var,b,i,ADCValue, ADCValueAvg,cmp;
float OutputVoltage, temp;
char str[10]="";

TEXTENTRY *my;
XCHAR* keypad[]={"1","2","3","4","5","6","7","8","9","0","<X","OK"};
XCHAR* numkeypad[]={"1","2","3","4","5","6","7","8","9","0","<X","OK"};
XCHAR  pin_entered[4]={0};
XCHAR  alarm_entered[5]={0};
XCHAR  PassWord[4]={'1','2','3','4'};


void CreatePinKeypad()
{
    GOL_SCHEME *mystylepin;
    mystylepin= GOLCreateScheme();
    mystylepin->TextColor0=YELLOW;
    mystylepin->Color1=YELLOW;
    mystylepin->TextColor1=BLACK;
   
    my = TeCreate(100,75,65,260,236,TE_DRAW,4,3,keypad,(XCHAR *)pin_entered,10,NULL,mystylepin);
    TeClearBuffer(my);
    SetCommandKeys(my);  
}

void CreateNumPinKeypad()
{
    GOL_SCHEME *mystylepin;
    mystylepin= GOLCreateScheme();
    mystylepin->TextColor0=YELLOW;
    mystylepin->Color1=YELLOW;
    mystylepin->TextColor1=BLACK;
   
    my = TeCreate(200,75,65,260,236,TE_DRAW,4,3,numkeypad,(XCHAR *)alarm_entered,10,NULL,mystylepin);
    TeClearBuffer(my);
    SetCommandKeys(my);
}

void SetCommandKeys(TEXTENTRY *pTe)
{
    TeSetKeyCommand(pTe, 10, TE_DELETE_COM); //assign the 6th key to be a Delete command
    TeSetKeyCommand(pTe, 11, TE_ENTER_COM);  //assign the Enter command to Enter key
}

void CreateButtons()
{
    BtnCreate(1,
            7,170,73,239,
            0,
            BTN_DRAW,
            NULL,
            "Alarm",
            NULL
            );
   
    BtnCreate(2,
            83,170,149,239,
            0,
            BTN_DRAW,
            NULL,
            "Temp",
            NULL
            );
   
    BtnCreate(3,10,10,90,70,0,
            BTN_DRAW,
            NULL,
            "Setting",
            NULL
            );
   
    BtnCreate(4,
            235,50,311,119,
            0,
            BTN_DRAW,
            NULL,
            "PIN",
            NULL
            );
    BtnCreate(99,159,170,235 ,239,0,BTN_DRAW,NULL,"RESTART",NULL);

    GOL_SCHEME *mystyleclock;
    mystyleclock= GOLCreateScheme();
    mystyleclock->CommonBkColor=WHITE;
    mystyleclock->Color0=LIGHTBLUE;
    mystyleclock->Color1=BLACK;
   
    AcCreate(5,
            100,30,200,150,1,20,60,
            TRUE,
            AC_DRAW,
            NULL,
            mystyleclock
            );
}

void Delay_us(unsigned int delay)
{
    for (i = 0; i < delay; i++)
        {
        __asm__ volatile ("repeat #39");
        __asm__ volatile ("nop");
        }
}

void initADC()
{  
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    ANSELBbits.ANSB9 = 1;// Ensure AN5/RB5 is analog
    _TRISB9 = 1;
    AD1CON1bits.AD12B = 0;
    AD1CON2bits.VCFG = 0; //using AVDD-which are greater of vdd-0.3 or 3.0, lesser of vdd+0.3 or 3.6, AVSS- vss-0.3 to vss+0.3
    AD1CON3bits.ADRC = 1; //using internal RC oscillator
    AD1CHS0bits.CH0SA = 9;// Select AN9/RB9 for CH0 +ve input
    AD1CHS0bits.CH0NA = 0;// Select Vref- for CH0 -ve input
    AD1CON2bits.CHPS = 0; //only S&H CH0 is selected
    AD1CON1bits.SIMSAM = 0; //samples multiple channels sequentially
    AD1CSSH = 0x0000; //input scanning
    AD1CSSL = 0x0000; //input scanning
    AD1CON1bits.ASAM = 1; //SAMP bit is automatically set that is sample enable bit
    AD1CON1bits.SSRCG = 0;
    AD1CON1bits.SSRC = 0b111; //automatic sample and automatic conversion
    AD1CON3bits.SAMC = 0b00011;
    AD1CON1bits.FORM = 0b00;
    AD1CON4bits.ADDMAEN = 0;//0 = Conversion results stored in ADCxBUF0 through ADCxBUFF registers; DMA is not used
    AD1CON2bits.SMPI = 0b01111; //when the buffer is full with 16 converted values than interrupt flag is set
    IFS0bits.AD1IF = 0;
    IPC3bits.AD1IP = 0;// don't know about the priority <2:0> ?
    IEC0bits.AD1IE = 1;
    AD1CON1bits.ADON = 1;
    Delay_us( 20);
}

void Temperature()
{
    initADC();
    int adc=1;
   
    while(adc)
    {
        Delay_us(100);// Sample for 100 us //remove this if required
        while(!AD1CON1bits.DONE);// Wait for the conversion to complete
        AD1CON1bits.DONE = 0;// Clear conversion done status bit
        if(IFS0bits.AD1IF == 1)
        {
            ADCValue = ADC1BUF0 + ADC1BUF1 + ADC1BUF2 + ADC1BUF3 + ADC1BUF4 + ADC1BUF5 +  ADC1BUF6 + ADC1BUF7 + ADC1BUF8 + ADC1BUF9 + ADC1BUFA + ADC1BUFB + ADC1BUFC + ADC1BUFD + ADC1BUFE + ADC1BUFF;// Read the ADC conversion result (sir asked to average all the 16 values)
            ADCValueAvg = ADCValue/16;
            OutputVoltage = 3.26 * ADCValueAvg;
            temp = (OutputVoltage-500)/10;
            IFS0bits.AD1IF = 0;
               
            IEC0bits.AD1IE= 0;
            adc=0;
        break;
        }
    }  
    
    BtnCreate(8,
        90,80,230,160,
        0,
        BTN_DRAW,
        NULL,
        "Click Me!",
        NULL);    
}

int XcharStrCmp(XCHAR *pCmp1, XCHAR *pCmp2, int len)
{
    int counter = 0;
    while(counter < len)
    {
        if(*pCmp1++ != *pCmp2++)
            return (0);
        counter++;
    }  
    return (1);
}

void main_page()
{
    LCDClear();
    LCDPutString("                       ");
    LCDPutString("         Please Enter PIN:");
    CreatePinKeypad();
}

WORD GOLMsgCallback(WORD objMsg, OBJ_HEADER* pObj, GOL_MSG *pMsg)
{
switch(GetObjID(pObj))
    {
     case 1:  //alarm
           DelayMs(1000);
           GOLFree();
           LCDClear();          
           Alarm();
        break;
           
    case 2: //Temperature
            DelayMs(1000);
            GOLFree();
            LCDClear();
            Temperature();
        break;
        
    case 4://change pin screen
            DelayMs(1000);
            GOLFree();
            LCDClear();
            LCDPutString("                     ");
            LCDPutString("             Enter New PIN");
            pinapp_active=1;
            draw_keypad();
            BtnCreate(44,250,180,310,230,0,BTN_DRAW,NULL,"Done",NULL); 
        break;
        
    case 44://done after changing pin
            DelayMs(1000);
            pinapp_active=0;
            GOLFree();
            LCDClear();
            CreateButtons();
            //while(1);
        break;

    case 11: //set key pressed in alarm screen
            DelayMs(1000);
            GOLFree();
            LCDClear();
            alarmapp_active=1;
            User_SetAlarm();
        break;
       
    case 12: //reset key pressed in alarm screen 
            DelayMs(1000);
            GOLFree();
            LCDClear();
            User_ResetAlarm();      
        break;
   
    case 111: //DONE
            DelayMs(1500);
            GOLFree();        
            LCDClear();
            alarmapp_active=0;
            AlarmVal_set();
 
    case 112://BACK
            DelayMs(1500);
            GOLFree();
            LCDClear();
            alarmapp_active=0;
            CreateButtons();
        break;
     
    case 3: //Setting
            DelayMs(1500);
            GOLFree();
            LCDClear();
            BtnCreate(31,80,20,260,120,0,BTN_DRAW,NULL,"Change Color",NULL);
        break;
        
    case 99:
            DelayMs(1500);
            main_page();
        break;
                    
    case 31: //"change color" next screen
            DelayMs(1500);
            GOLFree();
            LCDClear();
         
            LCDCenterString(0,"  Select");
            LCDCenterString(1,"  color");
            BtnCreate(311,10,10,110,110,0,BTN_DRAW,NULL,"YELLOW",NULL);
            BtnCreate(312,210,10,310,110,0,BTN_DRAW,NULL,"BLUE",NULL);
            BtnCreate(313,10,130,110,230,0,BTN_DRAW,NULL,"GREEN",NULL);
            BtnCreate(314,210,130,310,230,0,BTN_DRAW,NULL,"WHITE",NULL);
        break;
       
    case 311://yellow 
            DelayMs(1500);
            LCDSetBackground( YELLOW);
            GOLFree();
            LCDClear(); 
            CreateButtons();
        break;
   
    case 312: //blue color
            DelayMs(1500);
            LCDSetBackground( BLUE);     
            GOLFree();
            LCDClear(); 
            CreateButtons();
        break;
       
    case 313: //green color
            DelayMs(1500);
            LCDSetBackground( GREEN);
            GOLFree();
            LCDClear();   
            CreateButtons();
        break;
       
    case 314: //white color
            DelayMs(1500);
            LCDSetBackground( WHITE);          
            GOLFree();
            LCDClear();
            CreateButtons();
        break;
        
    case 100: //the keypad case (to press pins)
        if((my)->pActiveKey->index == 11){
           
            if(XcharStrCmp(TeGetBuffer((TEXTENTRY *)pObj), (XCHAR *)PassWord, PASSWORD_LEN) == 1)
                {
                    DelayMs(1000);
                    GOLFree();
                    LCDClear();
                    CreateButtons();
                }
                       
            else
                {
                    DelayMs(1000);
                    LCDClear();
                    GOLFree();
                    LCDCenterString(-2," WRONG PIN ENTERED");
                    LCDCenterString(-1,"Enter PIN again");
                    BtnCreate(101,130,140,190,180,0,BTN_DRAW,NULL,"OK",NULL);                
                }    
        }
        break;

    case 60:
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
    case 66:
    case 67:
    case 68:
    case 69:                
        
        if(pinapp_active==1)
            {   
                PassWord[index]=0;
                PassWord[index]=((GetObjID(pObj))%60)+48;
                index++;
                DelayMs(500);
            }
               
        else if(alarmapp_active==1)
                {
                   if(GetObjID(pObj)==60)
                   {
                       _AMASK = 0b0000;
                   }
                   else if(GetObjID(pObj)==61)
                   {
                       _AMASK = 0b0001;
                   }
                   else if(GetObjID(pObj)==62)
                   {
                       _AMASK = 0b0010;
                   }
                   else if(GetObjID(pObj)==63)
                   {
                       _AMASK = 0b0011;
                   }
                   else if(GetObjID(pObj)==64)
                   {
                       _AMASK = 0b0100;
                   }
                   else if(GetObjID(pObj)==65)
                   {
                       _AMASK = 0b0101;
                   }
                   else if(GetObjID(pObj)==66)
                   {
                       _AMASK = 0b0110;
                   }
                   else if(GetObjID(pObj)==67)
                   {
                       _AMASK = 0b0111;
                   }
                   else if(GetObjID(pObj)==68)
                   {
                       _AMASK = 0b1000;
                   }
                   else if(GetObjID(pObj)==69)
                   {
                       _AMASK = 0b1001;
                   }
                    DelayMs(800);
                }
        
                else
                {
                    //do nothing
                }
                  
        break;

               /* alarm_entered[alarm_index]=(GetObjID(pObj))%60;
                alarm_index++;*/
                //DelayMs(800);
               // break;
       
    case 8://Display Temperature
            DelayMs(1000);
            GOLFree();
            LCDClear();
            sprintf(str,"%.2f",temp);
            LCDSetColor( YELLOW);
            LCDCenterString(-1," Currently Temperature is:");
            LCDCenterString(0, str);
            LCDCenterString(1, "Degrees");    
            DelayMs(4000);
            GOLFree();
            LCDClear();          
            CreateButtons();
        break;

    case 101: //when wrong pin is enetered, this will direct back to keypad screen
            GOLFree();
            main_page();
        break;
     
    default: 
        break;
    }
return 1;
}

WORD GOLDrawCallback()
{
    return 1;
}

void TickInit(unsigned period_ms)
{
    TMR3 = 0;
    PR3 = TICK_PERIOD( period_ms);
    T3CONbits.TCKPS = 1;
    IFS0bits.T3IF = 0;
    IEC0bits.T3IE = 1;
    T3CONbits.TON = 1;
}

void __ISR _T3Interrupt(void)
{
    _T3IF = 0;
    TouchDetectPosition();
}

void Init()
{  
    LCDInit();
    LCDHome();
    uMBInit();
    TickInit( 1);
    GOLInit();
    InitGraph();
    DisplayBacklightOn();
    TouchInit(NVMWrite,NVMRead, NVMSectorErase, NULL);
}

void RTCInit()
{
    __builtin_write_OSCCONL(2);
    __builtin_write_RTCWEN();
    // Set the RTCWREN bit    
    _RTCEN = 0; // disable the module
    // disable alarm
    _ALRMEN = 0;
    if(RTCVAL==0)
   {
    // example set 2/10/2020 mon 22:22:30
    _RTCPTR = 3; // start the loading sequence
    RTCVAL = 2020; // YEAR
    RTCVAL = 0x032; // MONTH-1/DAY-1 //code u give ur date stuff and give the current time when u load
    RTCVAL = 0x0002; // WEEKDAY/HOURS
    RTCVAL = 0x2000; // MINUTES/SECONDS
   
    // set the ALARM for a specific day (my birthday)
    //DelayMs(1000);
    _ALRMPTR = 2; // start the loading sequence
    ALRMVAL = 0x0325; // MONTH-1/DAY-1// Make sure here date month weekday matches rtc value . alram hour and min , give ur wish time. and give sec=0;
    ALRMVAL = 0x0002; // WEEKDAY/HOURS    
    ALRMVAL = 0x2100; // MINUTES/SECONDS1
   
     // enable and lock
    _RTCEN = 1; // enable the module
    _RTCWREN = 0; // lock settings
    }     
    _RTCEN = 1; // enable the module
    _RTCIF=0;
    _RTCIE=0; 
}
   
void Alarm()
{  
BtnCreate(11,
        50,80,120,160,
        0,
        BTN_DRAW,
        NULL,
        "SET",
        NULL);
BtnCreate(12,
        150,80,220,160,
        0,
        BTN_DRAW,
        NULL,
        "RESET",
        NULL);
}

void User_SetAlarm()
{
    DelayMs(1000);
        RTCInit();
     
      draw_keypad();  
    BtnCreate(111,
        100,10,150,60,
        0,
        BTN_DRAW,
        NULL,
        "DONE",
        NULL);
   
    BtnCreate(112,
        155,10,205,60,
        0,
        BTN_DRAW,
        NULL,
        "BACK",
        NULL);
}

void User_ResetAlarm()
{ 
  // RTCC_ALM_Reset();
  _ALRMPTR=1;
  ALRMVAL = 0X0000; // MONTH-1/DAY-1
  ALRMVAL=0x0000;  
  // disable alarm
  _ALRMEN = 0;
  _RTCIF = 0; // clear interrupt flag
  LCDClear();
  // LCDSetBackground(BRIGHTRED);
  CreateButtons();
}

void AlarmVal_set()
{
    _ARPT = 0x0; // once //alarm must repeat 7 times
    _CHIME = 1; // indefinitely
    //  _AMASK = 0b1001; // set alarm once a year //half a second
    _ALRMEN = 1; // enable alarm
    _RTCIF = 0; // clear interrupt flag
    _RTCIE = 1; // enable interrupt
    //Clear_keysdata();
}
       
void  RTCC_ALM_Reset()
{
    _ALRMPTR=1;
    ALRMVAL = 0X0000; // MONTH-1/DAY-1
    ALRMVAL=0x0000;
   
    // disable alarm
    _ALRMEN = 0;
    _RTCIF = 0; // clear interrupt flag
}

void _ISR _RTCCInterrupt(void)
{
  _RTCIF = 0;
  _ALRMEN = 0;
  _ALRMPTR=1;
  ALRMVAL = 0X0000; // MONTH-1/DAY-1
  ALRMVAL=0x0000;
  alarm_index=0;
  LCDSetColor(YELLOW);
  LCDCenterString(-1," ALARM IS BUZZING");
  DelayMs(6000);
  LCDClear();
  LCDSetColor( RED);
  CreateButtons();
 // RTCCInterrupt
}

void msgdisplay(char *str)
{
    LCDClear();
    LCDSetBackground(YELLOW);
    LCDCenterString(0, str);
    BtnCreate(81,130,170,190,200,0,BTN_DRAW,NULL,"OK",NULL);
}

void draw_keypad()
{ 
       BtnCreate(60,
        10,100,30,160,
        0,
        BTN_DRAW,
        NULL,
        "0",
        NULL);
       BtnCreate(61,
        50,100,70,160,
        0,
        BTN_DRAW,
        NULL,
        "1",
        NULL);
       BtnCreate(62,
        80,100,100,160,
        0,
        BTN_DRAW,
        NULL,
        "2",
        NULL);
       BtnCreate(63,
        110,100,130,160,
        0,
        BTN_DRAW,
        NULL,
        "3",
        NULL);
       BtnCreate(64,
        140,100,160,160,
        0,
        BTN_DRAW,
        NULL,
        "4",
        NULL);
       BtnCreate(65,
        170,100,190,160,
        0,
        BTN_DRAW,
        NULL,
        "5",
        NULL);
       BtnCreate(66,
        200,100,220,160,
        0,
        BTN_DRAW,
        NULL,
        "6",
        NULL);
       BtnCreate(67,
        230,100,250,160,
        0,
        BTN_DRAW,
        NULL,
        "7",
        NULL);
       BtnCreate(68,
        260,100,280,160,
        0,
        BTN_DRAW,
        NULL,
        "8",
        NULL);
       BtnCreate(69,
        290,100,320,160,
        0,
        BTN_DRAW,
        NULL,
        "9",
        NULL);  
}

int main(void)
{
    Init();
    LCDSetBackground( RED);
    LCDSetColor( YELLOW);
    LCDCenterString(0, "Hi, How are you today?");
    LCDCenterString(1, " Loading...");
    DelayMs(4000);
    LCDClear();
    main_page();
    IEC0bits.AD1IE= 0;
    _RTCIE=0;
    while(1){
        if (GOLDraw()){
            TouchGetMsg( &msg);
            GOLMsg( &msg);
        }
    }
    return 0;
}
