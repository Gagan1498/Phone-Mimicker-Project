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
void Alarm(); 
void Set_Alarm();
void Reset_Alarm();
void Set_Alarm();
void Temperature();
void MyGOLDraw();
void initADC(void);
void CreateButtons(); 
void SetCommandKeys(TEXTENTRY *pTe);
void CreatePinKeypad_pinchange();

GOL_MSG msg; 
int  b,i,ADCValue, ADCValueAvg,cmp;
float OutputVoltage, temp;
char str[10]="";
TEXTENTRY *my;
XCHAR* keypad[]={"1","2","3","4","5","6","7","8","9","0","<X","OK"};
XCHAR  pin_entered[5]={0};
static XCHAR  PassWord[5] = {'3','6','5','4',0};
//XCHAR  *newpassword= PassWord;

void SetCommandKeys(TEXTENTRY *pTe)
{
    TeSetKeyCommand(pTe, 10, TE_DELETE_COM); //assign the 6th key to be a Delete command
    TeSetKeyCommand(pTe, 11, TE_ENTER_COM);  //assign the Enter command to Enter key
}

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
    
    BtnCreate(3,
            159,170,225,239,
            0,
            BTN_DRAW,
            NULL,
            "Setting",
            NULL
            );
    
    BtnCreate(4,
            235,170,311,239,
            0,
            BTN_DRAW,
            NULL,
            "Contact",
            NULL
            );

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
            adc=0;
            //while(1);
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


void Reset_Alarm() //what needs to be deleted??
{
    //GOLDeleteObjectByID(2);
    //GOLDeleteObjectByID(3);
    //Redraw_mainpage();
}

void Set_Alarm()
{
    BtnCreate(111,
        120,110,200,190,
        0,
        BTN_DRAW,
        NULL,
        "0000",
        NULL);
    
    BtnCreate(112,
        10,110,100,190,
        0,
        BTN_DRAW,
        NULL,
        "DONE",
        NULL);
    
    BtnCreate(113,
        220,110,310,190,
        0,
        BTN_DRAW,
        NULL,
        "BACK",
        NULL);
}

void  Alarm()
{ 
BtnCreate(11,
        10,10,100,100,
        0,
        BTN_DRAW,
        NULL,
        "SET",
        NULL);

BtnCreate(12,
        220,10,310,100,
        0,
        BTN_DRAW,
        NULL,
        "RESET",
        NULL);
} 

void main_page(){
    LCDClear();
    LCDPutString("                       ");
    LCDPutString("         Please Enter PIN:");
    CreatePinKeypad();
}

WORD GOLMsgCallback(WORD objMsg, OBJ_HEADER* pObj, GOL_MSG *pMsg)
{
switch(GetObjID(pObj)){
    case 1://ALARM         
            Alarm();
        break;
            
    case 2: //TEMP
            Temperature();
        break;
            
    case 11: //SET_ALARM
            Set_Alarm();
        break;
 
    case 12://RESET
            Reset_Alarm();
        break;
    
    case 3: //Setting
            GOLDeleteObjectByID(1);
            GOLDeleteObjectByID(2);
            GOLDeleteObjectByID(3);
            GOLDeleteObjectByID(4);
            GOLDeleteObjectByID(5);
            LCDClear();
            LCDSetBackground( RED);
            BtnCreate(31,80,65,260,160,0,BTN_DRAW,NULL,"Change PIN",NULL);
        break;

    case 31://after pressing Change pin
            LCDClear();
            LCDPutString("                       ");
            LCDPutString("         Enter New PIN:");
            CreatePinKeypad();
            int p;
            PassWord[5]={0};
            
            if(((TEXTENTRY *)pObj)->pActiveKey->index == 11){
            for(p=0;p<5;p++){
                PassWord[p]=pin_entered[p];
            }}
            while(1);
        break;
        
    case 8://Display Temperature
            GOLDeleteObjectByID(8);
            LCDClear();
            LCDSetBackground( GREEN);
            sprintf(str,"%.2f",temp);
            LCDSetColor( RED);
            LCDCenterString(-1," Currently Temperature is:");
            LCDCenterString(0, str);
            LCDCenterString(1, "Degrees");        
            BtnCreate(81,130,170,190,200,0,BTN_DRAW,NULL,"OK",NULL);
        break;
    
    case 81: //Case for "OK" at temperature screen
            GOLDeleteObjectByID(81);
            LCDClear();
            LCDSetBackground(BRIGHTRED);
            CreateButtons();
        break;
          
    case 100: //the keypad case (to press pins)
            if(((TEXTENTRY *)pObj)->pActiveKey->index == 11){
            
                if(XcharStrCmp(TeGetBuffer((TEXTENTRY *)pObj), (XCHAR *)PassWord, PASSWORD_LEN) == 1)
                    {
                        GOLDeleteObjectByID(100);
                        LCDClear();
                        CreateButtons(); 
                    }
                        
                else
                    {
                        LCDClear();
                        GOLDeleteObjectByID(100); 
                        LCDCenterString(-2," WRONG PIN ENETERED");
                        LCDCenterString(-1,"Enter PIN again");
                        BtnCreate(101,130,140,190,180,0,BTN_DRAW,NULL,"OK",NULL);                
                    }    
            }
        break;
        
    case 101: //when wrong pin is enetered, this will direct back to keypad screen
            GOLDeleteObjectByID(101);
            main_page();
        break;
      
    default: break;
}
return 1;
}

WORD GOLDrawCallback(){
    return 1;
}

void TickInit(unsigned period_ms){
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

void Init(){   
    LCDInit();
    LCDHome();
    uMBInit();
    TickInit( 1);
    GOLInit();
    InitGraph(); 
    DisplayBacklightOn();
    TouchInit(NVMWrite,NVMRead, NVMSectorErase, NULL);
}

int main(void)
{
    Init();
    LCDSetBackground(BRIGHTRED);
    LCDCenterString(0, "Hi, Welcome");
    LCDCenterString(1, " Loading...");
    DelayMs(4000);
    
    main_page();
    
    while(1){
        if (GOLDraw()){
            TouchGetMsg( &msg);
            GOLMsg( &msg);
        }
    }
    return 0;
}







